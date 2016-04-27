
all: dfs_client_upload dfs_client_download dfs_tracker

dfs_client_upload: dfs_client_upload.c
	gcc -g -Wall dfs_client_upload.c -o dfs_client_upload

dfs_client_download: dfs_client_download.c
	gcc -g -Wall dfs_client_download.c -o dfs_client_download

dfs_tracker: dfs_tracker.c
	gcc -g -Wall dfs_tracker.c -o dfs_tracker

clean:
	rm -f dfs_client_upload dfs_client_download dfs_tracker
