const assert=require('assert'),fs=require('fs'),path=require('path');
const {createHash}=require('crypto');
const ROOT=path.resolve(__dirname,'../..');
const {serve,boot,chromium}=require(ROOT+'/tools/browser/felib');
const {installMenuInput}=require(ROOT+'/tools/browser/menu_input');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
assert(process.argv.length===4 || process.argv.length===5,'usage: BUILD OUTPUT [INPUT_PLAN]');
const {execFileSync}=require('child_process');
const plan=process.argv[4] ? JSON.parse(fs.readFileSync(process.argv[4])) : JSON.parse(execFileSync('python3',[ROOT+'/tools/verify_driver_name_ui.py','plan'],{encoding:'utf8'}));
const layout=JSON.parse(fs.readFileSync(ROOT+'/tools/championship_save_layout.json'));
assert(output.startsWith('/tmp/wasm-dd2/'));fs.mkdirSync(output);
const sha=b=>createHash('sha256').update(b).digest('hex');
const card=fs.readFileSync(ROOT+'/DestructionDerby2/SaveGames');
const report={scope:'Real browser driver-name UI through trusted Playwright keys and read-only UI/canvas observations. No new lap record, persistence writes, races or whole original A/V acceptance.',target:'browser',pass_:false,engine_state_writes:false,input_keys:[],checkpoints:[],trusted_keyboard_events:[],binary_sha256:sha(fs.readFileSync(build+'/index.wasm')),helper_sha256:sha(fs.readFileSync(ROOT+'/tools/driver_name_input.py')),initial_card_sha256:sha(card),observer_sha256:sha(fs.readFileSync(__filename))};
const nav={Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape'};
async function ready(page,poly){await page.waitForFunction(poly=>HEAP16[0x46996c>>1]===0 && window.__nameReadyFrames>=16 && (poly===undefined || HEAPU32[0x940010>>2]===poly),poly);}
async function key(page,code){
  await ready(page);
  const mask={Left:0x80,Right:0x20,Up:0x10,Down:0x40,Return:0x4000,Escape:0x1000}[code];
  await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,mask);
  await page.keyboard.down(nav[code]);
  try{await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)!==0,mask);}
  finally{await page.keyboard.up(nav[code]);}
  await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)===0,mask);
  // Count fresh presentations of the current menu before another real key.
  // A menu pointer can change before its next slab animation starts.
  await page.evaluate(()=>{window.__nameReadyFrames=0;});
  await page.waitForTimeout(300);report.input_keys.push(code);
}
async function observed(page){return page.evaluate(layout=>{
  const d=new DataView(HEAPU8.buffer),hex=(a,n)=>Array.from(HEAPU8.subarray(a,a+n),b=>b.toString(16).padStart(2,'0')).join('');
  const state={joy_present:HEAPU8[0x754451]};
  for(const [name,a] of layout.fields)state[name]=d.getInt32(a,true);
  for(const [name,a,n] of layout.regions)state[name]=hex(a,n);
  return {state,ui:{menu:HEAPU32[0x940010>>2],cursor:[HEAP16[0x469f34>>1],HEAP16[0x469f36>>1]],entered:UTF8ToString(HEAPU32[0x469fd4>>2]+8),title:UTF8ToString(HEAPU32[0x46a024>>2]),names:hex(0x93e318,120),type:HEAP32[0x4673f4>>2],playable_tracks:HEAP32[0x467404>>2],playable_bowls:HEAP32[0x467408>>2]}};
},layout);}
async function checkpoint(page,name,cycle=false){
  await ready(page);const row={name,input_end:report.input_keys.length,...await observed(page)};
  if(cycle){
    const directory=output+'/'+name+'/cycle';fs.mkdirSync(directory,{recursive:true});
    await page.evaluate(()=>{window.__nameFrames=[];window.__captureName=true;});
    await page.waitForFunction(()=>!window.__captureName,null,{timeout:15000});
    const frames=await page.evaluate(()=>window.__nameFrames),metadata=[];
    assert(frames.length===64 && new Set(frames.map(f=>f.phase)).size===64);
    for(let i=0;i<frames.length;i++){
      const f=frames[i],prefix=`frame${String(i).padStart(3,'0')}`;
      assert(f.canvas_mismatches===0,'Actual canvas differs');
      for(const region of ['framebuf','palette'])fs.writeFileSync(directory+'/'+prefix+'-'+region+'.bin',Buffer.from(f[region],'base64'));
      const {framebuf,palette,...state}=f;metadata.push({...state,index:i,prefix});
    }
    fs.writeFileSync(directory+'/cycle.json',JSON.stringify({stage:'browser platform present',frames:metadata},null,2)+'\n');row.cycle=directory;
  }
  report.checkpoints.push(row);console.log(name,row.ui.entered);
}
(async()=>{
  const server=serve(build);await new Promise(r=>server.listen(0,r));let browser,watchdog;
  try{
    const disk=fs.statfsSync(output);assert(disk.bavail*disk.bsize>=1024**3);
    browser=await chromium.launch({args:['--no-sandbox']});watchdog=setTimeout(()=>browser.close().catch(()=>{}),600000);
    const page=await browser.newPage();await installMenuInput(page);
    await page.exposeFunction('observeNameKey',event=>report.trusted_keyboard_events.push(event));
    await page.addInitScript(()=>{
      window.__captureName=false;window.__nameReadyFrames=0;window.__nameLastMenu=null;
      for(const type of ['keydown','keyup'])window.addEventListener(type,e=>window.observeNameKey({type,code:e.code,trusted:e.isTrusted}));
      const present=CanvasRenderingContext2D.prototype.putImageData;
      const encode=(a,n)=>{let t='';for(let i=0;i<n;i+=16384)t+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(t);};
      CanvasRenderingContext2D.prototype.putImageData=function(...args){
        const result=present.apply(this,args);
        if(this.canvas.id==='canvas' && typeof HEAP16!=='undefined'){
          const menu=HEAPU32[0x940010>>2];
          window.__nameReadyFrames=HEAP16[0x46996c>>1]===0 ? (menu===window.__nameLastMenu ? window.__nameReadyFrames+1 : 1) : 0;
          window.__nameLastMenu=menu;
        }
        if(this.canvas.id==='canvas' && window.__captureName){
          let mismatches=0;const rgba=this.getImageData(0,0,640,480).data;
          for(let i=0;i<307200;i++){const pal=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;if(rgba[pixel]!==HEAPU8[pal]||rgba[pixel+1]!==HEAPU8[pal+1]||rgba[pixel+2]!==HEAPU8[pal+2]||rgba[pixel+3]!==255)mismatches++;}
          window.__nameFrames.push({phase:HEAP32[0x4699cc>>2],card_phase:HEAP32[0x467390>>2]%255,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],poly_list:HEAPU32[0x940010>>2],sound_volume:HEAP32[0x467410>>2],working_sound_volume:HEAP32[0x93fd20>>2],master_sfx_volume:HEAP32[0x462d84>>2],framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
          if(window.__nameFrames.length===64)window.__captureName=false;
        }return result;
      };
    });
    const errors=[];page.on('pageerror',e=>errors.push(e.message));await boot(page,server);
    async function keys(codes){for(const code of codes)await key(page,code);}
    async function opened(){await keys(['Return','Return','Return']);await ready(page,0x469f70);}
    async function enter(actions){for(const a of actions){await key(page,a.key);if(a.accepts||a.cancels)continue;const actual=(await observed(page)).ui;assert.deepStrictEqual(actual.cursor,a.cursor);assert.strictEqual(actual.entered,a.entered);}}
    await opened();await keys(['Right','Right','Right','Return','Down','Down','Right','Return']);
    assert.strictEqual((await observed(page)).ui.entered,'Dg');await checkpoint(page,'wrong-file-grid',true);
    await keys(['Escape','Escape','Escape']);await ready(page,0x4696b0);
    await opened();await checkpoint(page,'empty-grid',true);
    await enter(plan.limit.slice(0,-1));await checkpoint(page,'eight-character-limit',true);
    assert.strictEqual((await observed(page)).ui.entered,'MACSrPOO');await key(page,'Return');await ready(page,0x4696b0);
    await checkpoint(page,'accepted-limit-name');assert.strictEqual((await observed(page)).ui.playable_tracks,7);
    await opened();await enter(plan.deletion);await ready(page,0x4696b0);await checkpoint(page,'accepted-deletion');
    await opened();await enter(plan.cancel);await ready(page,0x46a624);await keys(['Escape','Escape']);await ready(page,0x4696b0);await checkpoint(page,'cancelled-name');
    await opened();await enter(plan.default);await ready(page,0x4696b0);await checkpoint(page,'accepted-default');
    const actual=await page.evaluate(()=>({raw:Array.from(FS.readFile('/SaveGames')),ram:Array.from(HEAPU8.subarray(0x754460,0x774460))}));
    assert(Buffer.from(actual.raw).equals(card)&&Buffer.from(actual.ram).equals(card));
    assert(errors.length===0,errors.join('; '));assert(report.trusted_keyboard_events.every(e=>e.trusted));
    report.pass_=true;report.card_unchanged=true;report.engine_matches_file=true;
  }catch(error){report.error=error.stack;throw error;}
  finally{clearTimeout(watchdog);fs.writeFileSync(output+'/report.json',JSON.stringify(report,null,2)+'\n');if(browser)await browser.close();await new Promise(r=>server.close(r));}
})();
