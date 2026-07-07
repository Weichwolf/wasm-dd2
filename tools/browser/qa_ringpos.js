// Read the live selection-ring coords on Config as the selection moves, to confirm they track the
// icons (icons at x ~135/320/505). Config ring dest = _DAT_00469070 (x) / _DAT_00469072 (y) WORD.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const ring=(page)=>page.evaluate(()=>{
  // signed 16-bit read
  const rd=a=>{let v=HEAPU16[a>>1]; return v>=32768?v-65536:v;};
  return {x:rd(0x469070), y:rd(0x469072), scr:HEAPU8[0x460005]};
}).catch(()=>'DEAD');
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:640,height:480}});
  await boot(page,server);
  await gotoButton(page,'Configuration'); await key(page,'Enter',1200);
  console.log('config icon0 (left):', JSON.stringify(await ring(page)));
  await key(page,'ArrowRight',900); console.log('config icon1 (mid): ', JSON.stringify(await ring(page)));
  await key(page,'ArrowRight',900); console.log('config icon2 (right):', JSON.stringify(await ring(page)));
  await key(page,'ArrowLeft',900);  console.log('config back to mid:  ', JSON.stringify(await ring(page)));
  await b.close(); server.close();
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
