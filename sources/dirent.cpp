/*
	File: dirent.cpp
	Author: Aggelos Kolaitis <neoaggelos@gmail.com>
	Description: Implements sync commands for the dirent.h interface
*/

#include "synctool.h"
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

const int MKDIR_MODE = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

const string SEP = "/";

bool fileExists(string file)
{
	struct stat s;
	int res = stat(file.c_str(), &s);
	return (res != -1 && S_ISREG(s.st_mode));
}

bool dirExists(string dir)
{
	struct stat s;
	int res = stat(dir.c_str(), &s);
	return (res != -1 && S_ISDIR(s.st_mode));
}

void assert_can_open_directory(string dir)
{
	cout << "Asserting '" << dir << "' exists and is accessible..." << endl;
	DIR *d = opendir(dir.c_str());

	if (d == NULL)
	{
		cout << "Error: Cannot open directory '" << dir << "'" << endl;
		die(EXIT_FAILURE);
	}

	closedir(d);
}

void create_subdirectories(string srcdir, string dstdir)
{
	DIR *src = opendir(srcdir.c_str());
	struct dirent *ent;

	while( (ent = readdir(src)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if(ent->d_type == DT_DIR)
			{
				string newdst = dstdir + SEP + ent->d_name;
				string newsrc = srcdir + SEP + ent->d_name;
				mkdir(newdst.c_str(), MKDIR_MODE);
				create_subdirectories(newsrc, newdst);
			}
		}
	}

	closedir(src);
}

void remove_missing_files(string srcdir, string dstdir)
{
	DIR *dst = opendir(dstdir.c_str());
	struct dirent *ent;

	while( (ent = readdir(dst)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if (ent->d_type == DT_DIR)
			{
				string newsrc = srcdir + SEP + ent->d_name;
				string newdst = dstdir + SEP + ent->d_name;
				remove_missing_files(newsrc, newdst);
			}
			else
			{
				string fileToCheck = srcdir + SEP + ent->d_name;
				if (!fileExists(fileToCheck))
				{
					string fileToRemove = dstdir + SEP + ent->d_name;
					remove(fileToRemove.c_str());
				}
			}
		}
	}

	closedir(dst);
}

void remove_missing_directories(string srcdir, string dstdir)
{
	DIR *dst = opendir(dstdir.c_str());
	struct dirent *ent;

	while( (ent = readdir(dst)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if (ent->d_type == DT_DIR)
			{
				string dirToCheck = srcdir + SEP + ent->d_name;
				if (!dirExists(dirToCheck))
				{
					string dirToDelete = dstdir + SEP + ent->d_name;
					remove_dir_recursive(dirToDelete);
				}
			}
		}
	}

	closedir(dst);
}

void remove_dir_recursive(string root)
{
	DIR *dir = opendir(root.c_str());
	struct dirent *ent;

	while( (ent = readdir(dir)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if (ent->d_type == DT_DIR)
			{
				string newroot = root + SEP + ent->d_name;
				remove_dir_recursive(newroot);
			}
			else
			{
				string fileToRemove = root + SEP + ent->d_name;
				remove(fileToRemove.c_str());
			}
		}
	}

	closedir(dir);
	remove(root.c_str());
}

void copy_all_files(string srcdir, string dstdir)
{
	DIR *src = opendir(srcdir.c_str());
	struct dirent *ent;

	while( (ent = readdir(src)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if (ent->d_type == DT_DIR)
			{
				string newsrc = srcdir + SEP + ent->d_name;
				string newdst = dstdir + SEP + ent->d_name;

				copy_all_files(newsrc, newdst);
			}
			else
			{
				string srcFile = srcdir + SEP + ent->d_name;
				string dstFile = dstdir + SEP + ent->d_name;

				/* delete old file to avoid complications */
				remove(dstFile.c_str());

				copy_file(srcFile, dstFile);
			}
		}
	}

	closedir(src);
}

void copy_new_and_updated_files(string srcdir, string dstdir)
{
	DIR *src = opendir(srcdir.c_str());
	struct dirent *ent;

	while( (ent = readdir(src)) )
	{
		if (string(ent->d_name) != "." && string(ent->d_name) != "..")
		{
			if (ent->d_type == DT_DIR)
			{
				string newsrc = srcdir + SEP + ent->d_name;
				string newdst = dstdir + SEP + ent->d_name;

				copy_all_files(newsrc, newdst);
			}
			else
			{
				string srcFile = srcdir + SEP + ent->d_name;
				string dstFile = dstdir + SEP + ent->d_name;

				if (!fileExists(dstFile))
				{
					copy_file(srcFile, dstFile);
				}
				else if (fileExists(dstFile) && fileIsNewer(srcFile, dstFile))
				{
					remove(dstFile.c_str());
					copy_file(srcFile, dstFile);
				}
			}
		}
	}

	closedir(src);
}

void copy_file(string srcFile, string dstFile)
{
	ifstream src(srcFile.c_str(), ios::binary);
	ofstream dst(dstFile.c_str(), ios::binary);

	dst << src.rdbuf();

	/* also copy permissions */
	struct stat st;
	if (stat(srcFile.c_str(), &st) != -1)
	{
		chmod(dstFile.c_str(), st.st_mode);
	}
}