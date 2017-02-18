# cs798-a2    
## To run server   
mkdir ./server   
./nfsfuse_server    

## To run client    
mkdir ./remote    
./nfsfuse_client -d -s ./remote    

## Remark   
- if client crashes, remember to umount the 'remote' directory    
- 
- 
