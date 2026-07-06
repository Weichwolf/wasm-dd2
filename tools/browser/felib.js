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
module.exports={serve,key,alive,rd,menuLabel,boot,gotoButton,PATHS,chromium};
