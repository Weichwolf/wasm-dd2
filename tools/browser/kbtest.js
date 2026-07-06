// Verify Finding 1 fixed: Config -> Control Method -> select Keyboard (icon 0) must NOT crash.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/kbtest'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[],crashed=false;
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  const step=async(k,l)=>{errs=[];await key(page,k,900);const a=await alive(page).catch(()=>false);console.log(`  ${l} (${k}): alive=${a} crashed=${crashed} errs=${errs.length} ${errs.length?JSON.stringify([...new Set(errs)]):''}`);await page.screenshot({path:`${OUT}/${l}.png`}).catch(()=>{});return a&&!crashed;};
  await gotoButton(page,'Configuration');
  await step('Enter','01_config');       // Configuration dialog
  await step('Enter','02_ctlmethod');    // Control Method dialog (Keyboard default-selected)
  await step('Enter','03_select_kbd');   // SELECT KEYBOARD -> previously crashed
  await page.waitForTimeout(1500);
  const finalAlive=await alive(page).catch(()=>false);
  console.log(`  after Keyboard select: alive=${finalAlive} crashed=${crashed}`);
  // navigate the key-config screen a bit + back out
  await step('Escape','04_back');
  console.log(`RESULT: ${finalAlive&&!crashed?'PASS (no crash)':'FAIL (crashed)'}`);
  await b.close(); server.close(); process.exit(finalAlive&&!crashed?0:2);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
