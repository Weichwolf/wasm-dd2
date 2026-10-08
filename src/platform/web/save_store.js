/* Direct IndexedDB backend for the owned C save store. Requests retain copied
 * JavaScript bytes, never C pointers; HEAP views are fetched only during poll.
 * One full card is one transaction record. Competing owners must reload. */
Module.dd2SaveBackend = (() => {
  const PENDING = 1, OK = 2, INVALID = 3, NO_MEMORY = 4, IO_ERROR = 5, CONFLICT = 6;
  const SIZE = 131072, TABLE = 'images', KEY = 'SaveGames';
  const records = new Map();
  let next = 0;
  function finish(record, status, data = null, exists = false) {
    record.status = status;
    record.data = data;
    record.exists = exists;
  }
  function begin(record) {
    record.status = PENDING;
    record.data = null;
  }
  function image(value) {
    return value instanceof ArrayBuffer && value.byteLength === SIZE
      ? new Uint8Array(value) : null;
  }
  function same(first, second) {
    if (first.length !== second.length) return false;
    for (let i = 0; i < first.length; ++i) if (first[i] !== second[i]) return false;
    return true;
  }
  function connect(record) {
    if (record.db) return Promise.resolve(record.db);
    return new Promise((resolve, reject) => {
      let settled = false;
      const request = indexedDB.open(record.name, 1);
      function error() {
        if (!settled) { settled = true; reject(new Error('Cannot open save database')); }
      }
      request.onblocked = error;
      request.onerror = error;
      request.onupgradeneeded = () => {
        if (!request.result.objectStoreNames.contains(TABLE)) request.result.createObjectStore(TABLE);
      };
      request.onsuccess = () => {
        const db = request.result;
        if (settled || !records.has(record.id)) { db.close(); return; }
        settled = true;
        record.db = db;
        db.onversionchange = () => { db.close(); record.db = null; };
        resolve(db);
      };
    });
  }
  async function read(record) {
    try {
      const db = await connect(record);
      const tx = db.transaction(TABLE, 'readonly');
      let status = OK, data = null, exists = false;
      const request = tx.objectStore(TABLE).get(KEY);
      request.onsuccess = () => {
        try {
          exists = request.result !== undefined;
          if (!exists) data = new Uint8Array(SIZE);
          else data = image(request.result);
          if (!data) { status = INVALID; tx.abort(); }
        } catch (error) { status = NO_MEMORY; tx.abort(); }
      };
      tx.oncomplete = () => finish(record, data ? status : IO_ERROR, data, exists);
      tx.onabort = () => finish(record, status === OK ? IO_ERROR : status);
      tx.onerror = () => {}; // onabort is the terminal result.
    } catch (error) { finish(record, IO_ERROR); }
  }
  function write(record, previous, replacement, existed) {
    try {
      if (!record.db) { finish(record, CONFLICT); return; }
      const tx = record.db.transaction(TABLE, 'readwrite', {durability: 'strict'});
      let status = OK;
      if (tx.durability !== 'strict') { status = IO_ERROR; tx.abort(); }
      const request = tx.objectStore(TABLE).get(KEY);
      request.onsuccess = () => {
        try {
          const currentExists = request.result !== undefined;
          const current = currentExists ? image(request.result) : null;
          if (currentExists && !current) status = INVALID;
          else if (currentExists !== existed || (existed && !same(current, previous))) status = CONFLICT;
          if (status !== OK) { tx.abort(); return; }
          tx.objectStore(TABLE).put(replacement.buffer, KEY);
        } catch (error) {
          status = IO_ERROR;
          tx.abort();
        }
      };
      tx.oncomplete = () => finish(record, OK, replacement, true);
      tx.onabort = () => finish(record, status === OK ? IO_ERROR : status);
      tx.onerror = () => {}; // Abort/QuotaExceeded preserves the previous record.
    } catch (error) { finish(record, IO_ERROR); }
  }
  return {
    create() {
      if (next >= 0x7fffffff) return 0;
      const id = ++next;
      records.set(id, {id, name: null, db: null, status: 0, data: null, exists: false});
      return id;
    },
    open(id, name) {
      const record = records.get(id);
      if (!record || record.status === PENDING) return false;
      record.name = name;
      begin(record);
      read(record);
      return true;
    },
    read(id) {
      const record = records.get(id);
      if (!record || record.status === PENDING) return false;
      begin(record);
      read(record);
      return true;
    },
    write(id, previous, replacement, existed) {
      const record = records.get(id);
      if (!record || record.status === PENDING) return false;
      begin(record);
      write(record, previous, replacement, existed);
      return true;
    },
    poll(id) { return records.get(id); },
    close(id) {
      const record = records.get(id);
      if (record && record.status === PENDING) return false;
      if (record && record.db) record.db.close();
      records.delete(id);
      return true;
    }
  };
})();
