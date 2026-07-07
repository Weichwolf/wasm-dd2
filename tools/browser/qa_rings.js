// Capture screenshots of the main menu + several sub-menus to inspect selection-ring placement.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const OUT='/tmp/rings'; require('fs').mkdirSync(OUT,{recursive:true});
const shot=(page,n)=>page.screenshot({path:`${OUT}/${n}.png`});
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:640,height:480}});
  await boot(page,server);
  await shot(page,'00_mainmenu'); console.log('main menu shot');
  // Race MODE dialog (Wrecking -> Enter)
  await gotoButton(page,'Wrecking'); await key(page,'Enter',1000); await shot(page,'01_racemode');
  await key(page,'ArrowRight',700); await shot(page,'01b_racemode_right'); console.log('race mode shot');
  // Race TYPE dialog
  await key(page,'Enter',1000); await shot(page,'02_racetype');
  await key(page,'ArrowRight',700); await shot(page,'02b_racetype_right'); console.log('race type shot');
  // back to menu, Configuration submenu
  await boot(page,server); await gotoButton(page,'Configuration'); await key(page,'Enter',1000); await shot(page,'03_config');
  await key(page,'ArrowDown',700); await shot(page,'03b_config_down'); console.log('config shot');
  // Control Method
  await key(page,'Enter',1000); await shot(page,'04_controlmethod');
  await key(page,'ArrowRight',700); await shot(page,'04b_controlmethod_right'); console.log('control method shot');
  // Sound Volume (Config -> down -> enter path may differ; just capture what we land on)
  await boot(page,server); await gotoButton(page,'Configuration'); await key(page,'Enter',900); await key(page,'ArrowDown',700); await key(page,'Enter',1000); await shot(page,'05_afterconfig2');
  await b.close(); server.close(); console.log('done -> '+OUT);
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
