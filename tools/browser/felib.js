// Shared FE navigation library: label-guided (spatial menu, not a fixed grid).
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
function serve(buildDir){
  return http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
    fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
}
const key=async(page,c,post=520)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(140);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rd=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<48;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
const menuLabel=(page)=>rd(page,0x46975c);   // main-menu selected button
// Race launch state. sb=screen byte @0x460005 (89 = in-race), lvl=current level @0x936ff4,
// cf=engine frame counter @0x462ff0 (must be advancing for a live race).
const raceState=(page)=>page.evaluate(()=>({sb:HEAPU8[0x460005],lvl:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],rt:HEAP32[0x4673f4>>2],nc:HEAP32[0x46765c>>2]})).catch(()=>({sb:-1,lvl:-1,cf:-1,rt:-1,nc:-1}));
// Robust launch predicate (qa_ttmp finding: single-sample `sb!=201` false-passes on transient
// dialog/loading frames — sb=41 flickers mid-race). Poll up to timeoutMs for the in-race screen
// sb==89 seen TWICE with the engine frame counter advancing. Returns {launched, ...lastState}.
async function waitRace(page,timeoutMs=25000){
  let prevCf=-1, hits=0, last=null;
  const t0=Date.now===undefined?0:0; // Date.now unused; loop-bounded below
  for(let i=0;i<Math.ceil(timeoutMs/1000);i++){
    const s=await raceState(page); last=s;
    if(s.sb===89 && s.lvl>0 && s.cf>prevCf){ if(++hits>=2) return {launched:true,...s}; }
    else hits=0;
    prevCf=s.cf;
    await page.waitForTimeout(1000);
  }
  return {launched:false,...(last||{})};
}
async function boot(page,server){
  await page.goto(`http://localhost:${server.address().port}/index.html?t=`+Math.floor(performance.now()),{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
}
// Verified fixed key-paths from a fresh boot (spatial menu: 2 rows x 4 cols).
// top:    Wrecking(race-opts) Select-Car   Select-Track   File-Manager
// bottom: CD-Audio-Player     Information   Configuration  Go!
const PATHS={
  'Wrecking':[], 'Select Car':['ArrowRight'], 'Select Track':['ArrowRight','ArrowRight'],
  'File Manager':['ArrowRight','ArrowRight','ArrowRight'],
  'CD Audio Player':['ArrowDown'], 'Information':['ArrowDown','ArrowRight'],
  'Configuration':['ArrowDown','ArrowRight','ArrowRight'], 'Go!':['ArrowDown','ArrowDown'],
};
// steer to a main-menu button whose label CONTAINS `want`, using its verified path (assumes at
// main menu / freshly booted). Verifies the resulting label.
async function gotoButton(page,want){
  const p=PATHS[want]; if(!p) return (await menuLabel(page)).includes(want);
  for(const k of p) await key(page,k,420);
  return (await menuLabel(page)).includes(want);
}
module.exports={serve,key,alive,rd,menuLabel,boot,gotoButton,raceState,waitRace,PATHS,chromium};
