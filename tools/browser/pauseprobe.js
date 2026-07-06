// Resolve Finding 2: read the RUNTIME pause-key binding + which key sets _pad_start/pause bit.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const u8=(page,va)=>page.evaluate(a=>HEAPU8[a],va).catch(()=>-1);
const u16=(page,va)=>page.evaluate(a=>HEAPU16?HEAPU16[a>>1]:(HEAPU8[a]|(HEAPU8[a+1]<<8)),va).catch(()=>-1);
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  // runtime keymap pause slot @0x46302d (_pad_start binding)
  console.log('runtime keymap _pad_start binding @0x46302d = 0x'+(await u8(page,0x46302d)).toString(16));
  console.log('  (0x0d=Enter, 0x1b=Escape)');
  // launch a race
  if(!await gotoButton(page,'Go!')){console.log('reach Go! failed');await b.close();server.close();return;}
  await key(page,'Enter',1500); await page.waitForTimeout(5000);
  const inrace=(await u8(page,0x460005))!==201;
  console.log('in race:',inrace);
  // press Enter, sample _pad_start(0x463040) + pause bit(0x75444a & 8)
  const sample=async(k)=>{
    await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),k);
    await page.waitForTimeout(60);
    const ps=await u8(page,0x463040); const pw=await u16(page,0x75444a);
    await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),k);
    await page.waitForTimeout(300);
    console.log(`  ${k} down: _pad_start=${ps} padword0x75444a=0x${(pw>>>0).toString(16)} pausebit(0x8)=${(pw&8)?'SET':'clear'}`);
  };
  await sample('Enter');
  await sample('Escape');
  await b.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
