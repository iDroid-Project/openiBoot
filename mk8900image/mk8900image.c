#include "nor_files.h"
#include <stdio.h>
#include <string.h>
#define BUFFERSIZE (1024*1024)

#ifdef __APPLE__
#include <gelf.h>
#else
#include <linux/elf.h>
#include <endian.h>
#endif

int
createImage(char* inElf, size_t inElfSize, char** outImage, size_t* outImageSize)
{
	Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*) inElf;
	size_t maxmem = 0;
	int i;

	if (strncmp(elf_hdr->e_ident, ELFMAG, 4) != 0 ) {
		return 0;
	}

	Elf32_Phdr* phdrs = (Elf32_Phdr*) (inElf + le32toh(elf_hdr->e_phoff));
	for (i = 0; i < le16toh(elf_hdr->e_phnum); i++) {
		int memExtent = le32toh(phdrs[i].p_vaddr) +
		    le32toh(phdrs[i].p_memsz) - le32toh(elf_hdr->e_entry);
		if (maxmem < memExtent)
			maxmem = memExtent;
	}

	if (!maxmem)
		return 0;

	*outImageSize = maxmem;
	*outImage = (char*) malloc(*outImageSize);
	if (*outImage == NULL)
		return 0;

	memset(*outImage, 0, *outImageSize);
	for (i = 0; i < le16toh(elf_hdr->e_phnum); i++) {
		memcpy(*outImage + le32toh(phdrs[i].p_vaddr) -
		    elf_hdr->e_entry, inElf + le32toh(phdrs[i].p_offset),
		    le32toh(phdrs[i].p_filesz));
	}

	return 1;
}

int
main(int argc, char* argv[])
{
	char* inElf;
	size_t inElfSize;
	char* outImage;
	size_t outImageSize;
	init_libxpwn();

	if (argc < 3) {
		printf("usage: %s <infile> <outfile> [template] [certificate]\n", argv[0]);
		return 0;
	}

	AbstractFile* template = NULL;
	if (argc >= 4) {
		template = createAbstractFileFromFile(fopen(argv[3], "rb"));
		if (!template) {
			fprintf(stderr, "error: cannot open template\n");
			return 1;
		}
	}

	AbstractFile* certificate = NULL;
	if (argc >= 5) {
		certificate = createAbstractFileFromFile(fopen(argv[4], "rb"));
		if (!certificate) {
			fprintf(stderr, "error: cannot open certificate\n");
			return 5;
		}
	}

	AbstractFile* inFile = openAbstractFile(createAbstractFileFromFile(fopen(argv[1], "rb")));
	if (!inFile) {
		fprintf(stderr, "error: cannot open infile\n");
		return 2;
	}

	AbstractFile* outFile = createAbstractFileFromFile(fopen(argv[2], "wb"));
	if (!outFile) {
		fprintf(stderr, "error: cannot open outfile\n");
		return 3;
	}

	AbstractFile* newFile;

	if (template) {
		if (certificate != NULL) {
			newFile = duplicateAbstractFile2(template, outFile, NULL, NULL, certificate);
		} else {
			newFile = duplicateAbstractFile(template, outFile);
		}
		if (!newFile) {
			fprintf(stderr, "error: cannot duplicate file from provided template\n");
			return 4;
		}
	} else {
		newFile = outFile;
	}

	inElfSize = (size_t) inFile->getLength(inFile);
	inElf = (char*) malloc(inElfSize);
	inFile->read(inFile, inElf, inElfSize);
	inFile->close(inFile);

	if (!createImage(inElf, inElfSize, &outImage, &outImageSize)) {
		/* It's a blob, just use the contents. */
		outImage = inElf;
		outImageSize = inElfSize;
	} else {
		/* It's an Elf, so use actual image data. */
		free(inElf);
	}

	newFile->write(newFile, outImage, outImageSize);
	newFile->close(newFile);

	free(outImage);

	return 0;
}
