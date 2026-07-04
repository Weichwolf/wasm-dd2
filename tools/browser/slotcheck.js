const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000);
  const slots=await page.evaluate(()=>{
    const r={};
    for(const a of [0x468054,0x468058,0x468068,0x46806c,0x46807c,0x468080]) r['0x'+a.toString(16)]=(HEAP32[a>>2]>>>0).toString(16);
    // compare: a known-working table slot (Front_End screen handler 0x4697cc)
    r['0x4697cc(ref)']=(HEAP32[0x4697cc>>2]>>>0).toString(16);
    r['0x4697d0(ref)']=(HEAP32[0x4697d0>>2]>>>0).toString(16);
    return r;
  });
  console.log(JSON.stringify(slots,null,0));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
