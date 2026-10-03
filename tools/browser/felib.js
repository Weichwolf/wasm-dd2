// Shared FE navigation library: label-guided (spatial menu, not a fixed grid).
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
function serve(buildDir){
  return http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
    fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
}
const key=async(page,c,post=520)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(140);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rd=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<48;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
const menuLabel=(page)=>rd(page,0x46975c);   // main-menu selected button
// 0x460005 is a byte of the rasterizer's cached CLUT pointer, not a screen ID.
// Keep sb as a legacy diagnostic; use the actual Play_Game level/quit/physics state.
const raceState=(page)=>page.evaluate(()=>({sb:HEAPU8[0x460005],lvl:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],rt:HEAP32[0x4673f4>>2],nc:HEAP32[0x46765c>>2],ticks:HEAP32[0x7746c0>>2],quit:HEAP32[0x7746ac>>2],demo:HEAP32[0x46385c>>2],replay:HEAP32[0x467074>>2]})).catch(()=>({sb:-1,lvl:-1,cf:-1,rt:-1,nc:-1,ticks:-1,quit:-1,demo:-1,replay:-1}));
// Require two consecutive increases of the physics tick in the same live level.
// Loading, dialogs and pauses cannot pass from a stale cf or texture-cache byte.
async function waitRace(page,timeoutMs=25000){
  let previous=null, hits=0, last=null;
  for(let i=0;i<Math.ceil(timeoutMs/1000);i++){
    const s=await raceState(page); last=s;
    if(s.lvl>=1 && s.lvl<=12 && s.quit===0 && s.demo===0 && s.ticks>0 &&
       previous && previous.lvl===s.lvl && previous.quit===0 && s.ticks>previous.ticks){ if(++hits>=2) return {launched:true,...s}; }
    else hits=0;
    previous=s;
    await page.waitForTimeout(1000);
  }
  return {launched:false,...(last||{})};
}
async function boot(page,server){
  await page.goto(`http://localhost:${server.address().port}/index.html?t=`+Math.floor(performance.now()),{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 && !!Module._dd2movieSource,null,{timeout:30000});
  await page.click('#canvas');await page.keyboard.press('Escape');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && !Module._dd2movieSource,null,{timeout:15000});
  await page.waitForTimeout(8000);
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
