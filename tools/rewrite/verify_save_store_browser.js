'use strict';
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, directory] = process.argv.slice(2);
if (!directory || !path.resolve(directory).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const input = JSON.parse(fs.readFileSync(path.join(directory, 'browser-input.json')));
const expected = input.images.map(value => Buffer.from(value, 'hex'));
const payloads = input.payloads.map(value => [...Buffer.from(value, 'hex')]);
const report = {scope: 'Actual public C store with Chromium IndexedDB, transaction abort/conflicts and browser-process reopen; no frontend/gameplay acceptance.', cases: [], pass_: false};
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
function check(condition, message) { if (!condition) throw new Error(message); }
let context;
const errors = [];
async function launch() {
  context = await chromium.launchPersistentContext(path.join(directory, 'chromium-profile'), {headless: true, args: ['--no-sandbox', '--disable-dev-shm-usage']});
  const page = await context.newPage();
  page.on('pageerror', error => errors.push(String(error)));
  await page.goto(url + '?reference=1');
  await page.waitForFunction(() => window.ready === true);
  return page;
}
async function call(page, name, args = [], types) {
  return page.evaluate(({name,args,types}) => Module.ccall('dd2_save_probe_'+name, 'number', types || args.map(()=>'number'), args), {name,args,types});
}
async function terminal(page, owner = 0) {
  await page.waitForFunction(owner => Module._dd2_save_probe_poll(owner) !== 1, owner);
  return call(page, 'poll', [owner]);
}
async function snapshot(page, owner, expectedBytes, label) {
  const bytes = Buffer.from(await page.evaluate(owner => {
    const size = Module._dd2_save_probe_size(owner), pointer = Module._dd2_save_probe_image(owner);
    return [...Module.HEAPU8.slice(pointer, pointer+size)];
  }, owner));
  check(bytes.equals(expectedBytes), label+' complete accepted memory image differs');
  report.cases.push({label, bytes: bytes.length, sha256: hash(bytes)});
}
async function disk(page, expectedBytes, label) {
  const bytes = Buffer.from(await page.evaluate(name => new Promise((resolve,reject) => {
    const request = indexedDB.open(name,1);
    request.onerror = () => reject(request.error);
    request.onsuccess = () => {
      const db=request.result, tx=db.transaction('images','readonly');
      let value;
      tx.objectStore('images').get('SaveGames').onsuccess = event => value=event.target.result;
      tx.oncomplete = () => {db.close(); resolve(value===undefined ? [] : [...new Uint8Array(value)]);};
      tx.onabort = () => {db.close();reject(tx.error);};
    };
  }), input.database));
  check(bytes.equals(expectedBytes), label+' independently read IndexedDB image differs');
  report.cases.push({label, bytes: bytes.length, sha256:hash(bytes)});
}
async function put(page, owner, logical, name, payload) {
  return page.evaluate(({owner,logical,name,payload}) => {
    const pointer = Module._malloc(payload.length);
    if (!pointer) throw new Error('Cannot allocate public input');
    Module.HEAPU8.set(payload,pointer);
    const accepted = Module.ccall('dd2_save_probe_put','number',['number','number','string','number','number'],[owner,logical,name,pointer,payload.length]);
    Module.HEAPU8.fill(0xee,pointer,pointer+payload.length);
    Module._free(pointer);
    return accepted;
  }, {owner,logical,name,payload});
}
(async () => {
  try {
    let page=await launch();
    check(await call(page,'open',[0,input.database],['number','string'])===1,'open rejected');
    check(await call(page,'close',[0])===0,'pending open destroyed');
    check(await terminal(page)===2,'empty open failed');
    await snapshot(page,0,expected[0],'empty-memory');
    await disk(page,Buffer.alloc(0),'missing-key-is-not-written');
    check(await put(page,0,0,'A',payloads[0])===1,'A begin failed');
    check(await call(page,'close',[0])===0,'pending write destroyed');
    await snapshot(page,0,expected[0],'A-unacknowledged-memory');
    check(await terminal(page)===2,'A commit failed');
    await snapshot(page,0,expected[1],'A-committed-memory');
    await disk(page,expected[1],'A-committed-database');
    check(await call(page,'open',[1,input.database],['number','string'])===1 && await terminal(page,1)===2,'second owner failed');
    check(await put(page,0,1,'B',payloads[1])===1 && await terminal(page)===2,'B failed');
    await disk(page,expected[2],'A-B-database');
    check(await put(page,1,1,'STALE',payloads[2])===1 && await terminal(page,1)===6,'stale owner silently wrote');
    check(await call(page,'phase',[1])===5,'stale owner does not require reload');
    await snapshot(page,1,expected[1],'stale-accepted-memory-preserved');
    await disk(page,expected[2],'stale-database-preserved');
    check(await call(page,'delete',[1,0])===0,'stale owner retried');
    check(await call(page,'reload',[1])===1 && await terminal(page,1)===2,'stale reload failed');
    await snapshot(page,1,expected[2],'stale-reload');
    await page.evaluate(() => {
      window.originalPut = IDBObjectStore.prototype.put;
      window.abortedWrites = 0;
      IDBObjectStore.prototype.put = function(...args) {
        const request=window.originalPut.apply(this,args);
        ++window.abortedWrites;
        this.transaction.abort();
        return request;
      };
    });
    check(await put(page,0,0,'FAIL',payloads[2])===1 && await terminal(page)===5,'real transaction abort not reported');
    check(await page.evaluate(()=>window.abortedWrites)===1,'native put was not enqueued');
    await snapshot(page,0,expected[2],'aborted-memory-preserved');
    await disk(page,expected[2],'aborted-database-preserved');
    await page.evaluate(()=>{IDBObjectStore.prototype.put=window.originalPut;});
    await page.evaluate(() => {
      window.rejectedEnqueues = 0;
      IDBObjectStore.prototype.put = function() {
        ++window.rejectedEnqueues;
        throw new DOMException('Injected enqueue failure', 'QuotaExceededError');
      };
    });
    check(await put(page,0,0,'FAIL',payloads[2])===1 && await terminal(page)===5,'synchronous enqueue exception not reported');
    check(await page.evaluate(()=>window.rejectedEnqueues)===1,'enqueue exception was not exercised');
    await snapshot(page,0,expected[2],'enqueue-exception-memory-preserved');
    await disk(page,expected[2],'enqueue-exception-database-preserved');
    await page.evaluate(()=>{IDBObjectStore.prototype.put=window.originalPut;});
    check(await call(page,'delete',[0,0])===1 && await terminal(page)===2,'delete failed');
    await snapshot(page,0,expected[3],'deleted-A-memory');
    check(await put(page,0,1,'C',payloads[2])===1 && await terminal(page)===2,'C failed');
    await snapshot(page,0,expected[4],'C-B-physical-compaction');
    await disk(page,expected[4],'C-B-database');
    check(await put(page,0,2,'B',payloads[3])===0 && await call(page,'poll',[0])===9,'duplicate accepted');
    check(await put(page,0,2,'SHORT',payloads[3].slice(1))===0 && await call(page,'poll',[0])===3,'truncated payload accepted');
    await disk(page,expected[4],'rejected-input-database');
    check(await call(page,'close',[0])===1 && await call(page,'close',[1])===1,'terminal owners leaked');
    await context.close(); context=null;
    page=await launch();
    check(await call(page,'open',[0,input.database],['number','string'])===1 && await terminal(page)===2,'process reopen failed');
    await snapshot(page,0,expected[4],'fresh-browser-process-reopen');
    const prefix=Buffer.from(expected[4]);prefix[0]=2;
    await page.evaluate(({name,bytes})=>new Promise((resolve,reject)=>{
      const request=indexedDB.open(name,1);
      request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readwrite',{durability:'strict'});tx.objectStore('images').put(new Uint8Array(bytes).buffer,'SaveGames');tx.oncomplete=()=>{db.close();resolve();};tx.onabort=()=>{db.close();reject(tx.error);};};
      request.onerror=()=>reject(request.error);
    }),{name:input.database,bytes:[...prefix]});
    check(await call(page,'reload',[0])===1 && await terminal(page)===3,'corrupt header loaded');
    await snapshot(page,0,expected[4],'corrupt-reload-preserves-memory');
    check(await call(page,'phase',[0])===5 && await call(page,'delete',[0,0])===0,'corrupt owner writable');
    check(await call(page,'close',[0])===1,'corrupt terminal owner not closed');
    check(await call(page,'open',[0,input.database],['number','string'])===1 && await terminal(page)===3,'corrupt reopen accepted');
    check(await call(page,'size',[0])===0 && await call(page,'phase',[0])===0,'corrupt open partial owner');
    async function inject(bytes) {
      await page.evaluate(({name,bytes})=>new Promise((resolve,reject)=>{
        const request=indexedDB.open(name,1);
        request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readwrite',{durability:'strict'});tx.objectStore('images').put(typeof bytes==='string'?bytes:new Uint8Array(bytes).buffer,'SaveGames');tx.oncomplete=()=>{db.close();resolve();};tx.onabort=()=>{db.close();reject(tx.error);};};
        request.onerror=()=>reject(request.error);
      }),{name:input.database,bytes});
    }
    for (const [label, value] of [['truncated',[...expected[4].subarray(1)]],['extra',[...expected[4],1]],['wrong-type','invalid']]) {
      check(await call(page,'close',[0])===1,'closed invalid owner not released');
      await inject(value);
      check(await call(page,'open',[0,input.database],['number','string'])===1 && await terminal(page)===3,label+' database loaded');
      check(await call(page,'size',[0])===0,label+' partial image published');
      report.cases.push({label:'rejected-'+label, result:3});
    }
    check(await call(page,'close',[0])===1,'invalid terminal owner not released');
    await inject([...expected[5]]);
    check(await call(page,'open',[0,input.database],['number','string'])===1 && await terminal(page)===2,'full original card did not open');
    check(await call(page,'count',[0])===15,'full cardinality differs');
    check(await put(page,0,15,'EXCESS',payloads[0])===0 && await call(page,'poll',[0])===8,'full append accepted');
    await disk(page,expected[5],'full-append-preserves-database');
    check(await put(page,0,7,'FULLNEW',payloads[0])===1 && await terminal(page)===2,'full replacement failed');
    await snapshot(page,0,expected[6],'full-replacement-memory');
    await disk(page,expected[6],'full-replacement-database');
    check(await call(page,'close',[0])===1,'completed owner not released');
    check(errors.length===0,'browser exceptions: '+errors.join('; '));
    report.pass_=true;report.browserVersion=context.browser()?.version();
  } finally {
    if(context) await context.close();
    fs.writeFileSync(path.join(directory,'browser-report.json'),JSON.stringify(report,null,2)+'\n');
  }
})().catch(error=>{console.error(error);process.exitCode=1;});
