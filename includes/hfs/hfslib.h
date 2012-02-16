/**
 * hfs/hfslib.h
 *
 * Copyright 2011 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "common.h"
#include "hfsplus.h"
#include "abstractfile.h"

#ifdef __cplusplus
extern "C" {
#endif
	void writeToFile(HFSPlusCatalogFile* file, AbstractFile* output, Volume* volume);
	void writeToHFSFile(HFSPlusCatalogFile* file, AbstractFile* input, Volume* volume);
	void get_hfs(Volume* volume, const char* inFileName, AbstractFile* output);
	int add_hfs(Volume* volume, AbstractFile* inFile, const char* outFileName);
	void grow_hfs(Volume* volume, uint64_t newSize);
	void removeAllInFolder(HFSCatalogNodeID folderID, Volume* volume, const char* parentName);
	void addAllInFolder(HFSCatalogNodeID folderID, Volume* volume, const char* parentName);
	void addall_hfs(Volume* volume, const char* dirToMerge, const char* dest);
	void extractAllInFolder(HFSCatalogNodeID folderID, Volume* volume);
	int copyAcrossVolumes(Volume* volume1, Volume* volume2, char* path1, char* path2);

	void hfs_untar(Volume* volume, AbstractFile* tarFile);
	void hfs_ls(Volume* volume, const char* path);
	void hfs_setsilence(int s);
#ifdef __cplusplus
}
#endif

