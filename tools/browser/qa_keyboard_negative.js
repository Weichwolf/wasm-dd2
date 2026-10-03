// Reproduce the genuine pre-extension shell filter with the unchanged current
// WASM, then require the same trusted-browser input test to reject dropped keys.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {execFileSync,spawnSync}=require('child_process');
const root=path.resolve(__dirname,'../..'),build=path.resolve(process.argv[2]||'web/dd2'),out=path.resolve(process.argv[3]);
fs.mkdirSync(out,{recursive:false});
const negative=path.join(out,'build');fs.mkdirSync(negative);
for(const entry of fs.readdirSync(build,{withFileTypes:true})){
 if(entry.name==='index.html')continue;
 const from=path.join(build,entry.name),to=path.join(negative,entry.name);
 if(entry.isDirectory())fs.symlinkSync(from,to);else fs.linkSync(from,to);
}
const original=fs.readFileSync(path.join(build,'index.html'),'utf8');
const old=execFileSync('git',['show','66cbff7:web/shell_port.html'],{cwd:root,encoding:'utf8'});
const begin='        // The original rebind scan list',end='        for (var _c=65;';
const a=original.indexOf(begin),b=original.indexOf(end,a),c=old.indexOf(begin),d=old.indexOf(end,c);
assert(a>=0 && b>a && c>=0 && d>c,'could not isolate the literal old shell key filter');
fs.writeFileSync(path.join(negative,'index.html'),original.slice(0,a)+old.slice(c,d)+original.slice(b));
const run=spawnSync(process.execPath,[path.join(__dirname,'qa_keyboard.js'),negative,path.join(out,'case')],{encoding:'utf8',timeout:120000});
fs.writeFileSync(path.join(out,'run.log'),(run.stdout||'')+(run.stderr||''));
assert(run.status===1 && run.stderr.includes('browser shell lost/misrouted declared key transitions'),'old shell was not rejected by the actual key boundary assertion');
const observed=JSON.parse(fs.readFileSync(path.join(out,'case/shell-transitions.json')));
assert(observed.length===46 && !observed.some(e=>e.code==='F3') && observed.some(e=>e.code==='F10'),'negative did not preserve old supported keys/drop new keys');
fs.writeFileSync(path.join(out,'report.json'),JSON.stringify({scope:'Actual 66cbff7 shell key filter, current unchanged WASM and normal original startup: trusted Chromium inputs must fail the positive test at its 104-transition bridge assertion.',wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),old_shell_commit:'66cbff7',observed_transitions:observed.length,expected_transitions:104,dropped_new_keys:58,negative_shell_rejected:true},null,2)+'\n');
console.log('PASS negative browser: genuine old shell drops 58 of 104 trusted transitions; same boundary test rejects it');
