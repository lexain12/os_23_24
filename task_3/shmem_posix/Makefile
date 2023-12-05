all:
	@gcc shmem_client_posix.c -lrt -pthread -o client
	@gcc shmem_server_posix.c -lrt -pthread -o server
	@dd if=/dev/urandom of=input.txt bs=1024 count=4
	@echo " >>>  Launch server in second terminal"
	@./client 
