'use strict';
// Literal source records, with a single bounded decoded block in memory.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),zlib=require('zlib'),assert=require('assert');
const BYTES=128+307200+2048;
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
class VideoRecords {
 constructor(directory,proof){
  this.directory=fs.realpathSync(directory);this.cached=-1;
  assert(this.directory.startsWith('/tmp/wasm-dd2/'),'original video escapes temporary workspace');
  const manifest=path.join(this.directory,'video-archive/manifest.json');
  if(fs.existsSync(manifest)){
   assert(!fs.existsSync(path.join(this.directory,'video.bin')),'ambiguous original video storage');
   const bytes=fs.readFileSync(manifest);
   if(proof)assert(hash(bytes)===proof.video_archive_manifest_sha256,'original video archive provenance differs');
   const m=this.manifest=JSON.parse(bytes);
   assert(m.pass_===true && m.version===1 && m.codec==='zlib' && m.record_bytes===BYTES &&
    Number.isInteger(m.frame_count) && m.frame_count>0 && m.frame_count<=60000 &&
    Number.isInteger(m.chunk_frames) && m.chunk_frames>0 && m.chunk_frames<=128,'invalid video archive');
   let total=0;
   for(const [i,c] of m.chunks.entries()){
    assert(c.file===String(i).padStart(6,'0')+'.zlib' && c.first===total &&
     Number.isInteger(c.frames) && c.frames>0 && c.frames<=m.chunk_frames &&
     (i===m.chunks.length-1 || c.frames===m.chunk_frames) && c.raw_bytes===c.frames*BYTES,'invalid video chunk index');
    total+=c.frames;
   }
   assert(total===m.frame_count && m.raw_bytes===total*BYTES,'incomplete video archive');
   if(proof)assert(proof.video_sha256===m.raw_sha256,'original archive stream provenance differs');
   this.count=total;
  }else{
   assert(!proof?.video_archive_manifest_sha256,'original archive manifest missing');
   const filename=path.join(this.directory,'video.bin'),size=fs.statSync(filename).size;
   assert(size>0 && size%BYTES===0 && size<=4096*BYTES,'incomplete or unbounded legacy video');
   this.count=size/BYTES;this.fd=fs.openSync(filename,'r');
  }
  if(proof)assert(proof.record_bytes===BYTES && proof.frame_count===this.count,'original video extent differs');
 }
 record(index){
  assert(Number.isInteger(index) && index>=0 && index<this.count,'invalid original video index');
  if(this.fd!==undefined){
   const raw=Buffer.alloc(BYTES);
   assert(fs.readSync(this.fd,raw,0,BYTES,index*BYTES)===BYTES,'incomplete original video record');
   return raw;
  }
  let lo=0,hi=this.manifest.chunks.length;
  while(lo+1<hi){const mid=(lo+hi)>>1;if(this.manifest.chunks[mid].first<=index)lo=mid;else hi=mid;}
  const c=this.manifest.chunks[lo];
  if(this.cached!==lo){
   const packed=fs.readFileSync(path.join(this.directory,'video-archive',c.file));
   assert(packed.length===c.compressed_bytes && hash(packed)===c.compressed_sha256,'compressed video block differs');
   const decoded=zlib.inflateSync(packed,{maxOutputLength:c.raw_bytes,info:true});
   assert(decoded.engine.bytesWritten===packed.length && decoded.buffer.length===c.raw_bytes &&
    hash(decoded.buffer)===c.raw_sha256,'decoded video block differs');
   this.data=decoded.buffer;this.cached=lo;
  }
  const offset=(index-c.first)*BYTES;
  return this.data.subarray(offset,offset+BYTES);
 }
 close(){if(this.fd!==undefined){fs.closeSync(this.fd);this.fd=undefined;}}
}
module.exports={VideoRecords,BYTES};
