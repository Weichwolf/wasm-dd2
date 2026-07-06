// Verify Name Entry now accepts letters after the char** fix.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/nameentry'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
const nameBuf=(page)=>page.evaluate(()=>{const p=HEAPU32[0x469fd4>>2];let s='';for(let i=0;i<12;i++){const c=HEAPU8[p+8+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'ERR');
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR',e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await gotoButton(page,'Wrecking');
  await key(page,'Enter',900); await key(page,'Enter',900); await key(page,'Enter',1200); // -> Name Entry
  console.log('at Name Entry, name="'+await nameBuf(page)+'"');
  await page.screenshot({path:`${OUT}/00_open.png`});
  // press Enter on the default cell (A) a few times; then navigate + accept different letters
  for(let i=0;i<3;i++){ await key(page,'Enter',700); console.log(`  Enter#${i}: name="${await nameBuf(page)}"`); }
  // navigate right a couple cells then accept (different letters)
  await key(page,'ArrowRight',600); await key(page,'Enter',700); console.log(`  Right,Enter: name="${await nameBuf(page)}"`);
  await key(page,'ArrowDown',600); await key(page,'Enter',700); console.log(`  Down,Enter: name="${await nameBuf(page)}"`);
  await page.screenshot({path:`${OUT}/01_typed.png`});
  const final=await nameBuf(page);
  console.log(`RESULT: name="${final}" -> ${final.length>0?'PASS (letters registered)':'FAIL (still blank)'}`);
  await b.close(); server.close(); process.exit(final.length>0?0:2);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
