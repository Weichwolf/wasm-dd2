// Reproduce the agent's Finding 1: Race Mode -> confirm -> Race Type -> ArrowRight x2 (3rd icon).
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/racetype'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[];
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  const step=async(k,l)=>{errs=[];await key(page,k,700);const a=await alive(page);const rtl=await rd(page,0x46a748).catch(()=>'');console.log(`  ${l} (${k}): alive=${a} errs=${errs.length} rtlabel="${rtl}" ${errs.length?JSON.stringify([...new Set(errs)]):''}`);await page.screenshot({path:`${OUT}/${l}.png`});};
  await gotoButton(page,'Wrecking');
  await step('Enter','01_race_mode');   // Race Mode dialog
  await step('Enter','02_race_type');   // confirm Wrecking -> Race Type dialog
  await step('ArrowRight','03_idx1');   // Single Race
  await step('ArrowRight','04_idx2');   // 3rd icon (Time Trial) <- agent's trap
  await step('ArrowRight','05_idx3');   // 4th icon (2-Player)
  await step('Enter','06_confirm');
  await b.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
