service NFS {
	
	void ping(),
	
	//fixme second parameter is output of struct type
	i32 lstat(1:string path, ),

}
