#include "nor_files.h"
#include <stdio.h>
#include <string.h>
#define BUFFERSIZE (1024*1024)

int main(int argc, char* argv[]) {
	char* inFileData;
	size_t inFileSize;
	init_libxpwn();

	if(argc < 3) {
		printf("usage: %s <infile> <outfile> [template] [certificate]\n", argv[0]);
		return 0;
	}

	AbstractFile* template = NULL;
	if(argc >= 4) {
		template = createAbstractFileFromFile(fopen(argv[3], "rb"));
		if(!template) {
			fprintf(stderr, "error: cannot open template\n");
			return 1;
		}
	}

	AbstractFile* certificate = NULL;
	if(argc >= 5) {
		certificate = createAbstractFileFromFile(fopen(argv[4], "rb"));
		if(!certificate) {
			fprintf(stderr, "error: cannot open certificate\n");
			return 5;
		}
	}

	AbstractFile* inFile = openAbstractFile(createAbstractFileFromFile(fopen(argv[1], "rb")));
	if(!inFile) {
		fprintf(stderr, "error: cannot open infile\n");
		return 2;
	}

	AbstractFile* outFile = createAbstractFileFromFile(fopen(argv[2], "wb"));
	if(!outFile) {
		fprintf(stderr, "error: cannot open outfile\n");
		return 3;
	}

	AbstractFile* newFile;

	if(template) {
		if(certificate != NULL) {
			newFile = duplicateAbstractFile2(template, outFile, NULL, NULL, certificate);
		} else {
			newFile = duplicateAbstractFile(template, outFile);
		}
		if(!newFile) {
			fprintf(stderr, "error: cannot duplicate file from provided template\n");
			return 4;
		}
	} else {
		newFile = outFile;
	}

	inFileSize = (size_t) inFile->getLength(inFile);
	inFileData = (char*) malloc(inFileSize);
	inFile->read(inFile, inFileData, inFileSize);
	inFile->close(inFile);

	newFile->write(newFile, inFileData, inFileSize);
	newFile->close(newFile);

	free(inFileData);

	return 0;
}

