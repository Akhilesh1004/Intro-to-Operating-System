/*
Student No.: 111705034
Student Name: 鄭秉豐
Email: rty78452@gmail.com
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;
static struct fuse_operations op;

struct TarfileHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];

    mode_t getMode() { return strtol(mode, nullptr, 8); }
    short getUid() { return strtol(uid, nullptr, 8); }
    short getGid() { return strtol(gid, nullptr, 8); }
    size_t getSize() { return strtol(size, nullptr, 8); }
    time_t getMtime() { return strtol(mtime, nullptr, 8); }
};

map<string, set<string>> file_directory;
map<string, struct stat *> file_attribute;
map<string, char *> file_content;
map<string, string> symlink_targets;

int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    string dir_path = path;
    //cout << "my_readdir called for path: " << dir_path << endl;
    if (dir_path[dir_path.size() - 1] != '/') dir_path += '/';
    if (file_directory.find(dir_path) == file_directory.end()) return -ENOENT; // Directory not found

    filler(buffer, ".", NULL, 0);  // Current directory
    filler(buffer, "..", NULL, 0); // Parent directory
    // Add regular files and directories
    for (string file: file_directory[dir_path]) {
        if (file[file.size() - 1] == '/') file.pop_back();;
        filler(buffer, file.c_str(), NULL, 0); // Add each filename, without /
    }
    return 0;
}
int my_getattr(const char *path, struct stat *st) {
    string file_path = path;
    if (file_path == "/") { // Root directory
        st->st_mode = S_IFDIR | 0444; // Read-only directory
        st->st_nlink = 2;
        return 0;
    }
    if (file_attribute.count(file_path) == 1) {
      memcpy(st, file_attribute[file_path], sizeof(struct stat));
      st->st_mode = S_IFREG | st->st_mode;
      return 0;
    }
    file_path += '/';
    if (file_attribute.count(file_path) == 1) {
      memcpy(st, file_attribute[file_path], sizeof(struct stat));
      st->st_mode = S_IFDIR | st->st_mode;
      return 0;
    }
    return -ENOENT;

}
int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    string file_path = path;
    if (file_content.find(file_path) == file_content.end()) return -ENOENT; // File not found

    size_t file_size = file_attribute[file_path]->st_size;
    if (offset >= file_size) return 0; // Offset beyond file size

    size_t bytes_to_read = min(size, static_cast<size_t>(file_size - offset));
    memcpy(buffer, file_content[file_path] + offset, bytes_to_read);
    return bytes_to_read;
}
int my_readlink(const char *path, char *buffer, size_t size) {
    string link_path = path;
    if (symlink_targets.find(link_path) == symlink_targets.end()) return -ENOENT; // Symbolic link not found
    string target = symlink_targets[link_path];
    strncpy(buffer, target.c_str(), size);
    return 0;
}

int main(int argc, char *argv[]) {
    fstream file("test.tar", ios::in | ios::binary);
    while (file) {
        TarfileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (header.name[0] == '\0') break;
        
        string name = header.name;
        size_t size = header.getSize();
        name = "/"+name;

        struct stat *st = new struct stat;
        st->st_mode = header.getMode();
        st->st_uid = header.getUid();
        st->st_gid = header.getGid();
        st->st_size = size;
        st->st_mtime = header.getMtime();
        st->st_nlink = 0;
        st->st_blocks = 0;

        if (header.typeflag[0] == '5') { // Directory
            if (name[name.size() - 1] != '/') {
                name += '/';
            }
            file_directory[name]; // Create directory entry
            size_t last_slash = name.find_last_of('/', name.size() - 2);
            string parent_dir = (last_slash == string::npos) ? "/" : name.substr(0, last_slash + 1);
            file_directory[parent_dir].insert(name.substr(last_slash + 1));
        } else if (header.typeflag[0] == '2') { // Symbolic link
            symlink_targets[name] = header.linkname;
            st->st_mode |= S_IFLNK;
            size_t last_slash = name.find_last_of('/', name.size() - 2);
            string parent_dir = (last_slash == string::npos) ? "/" : name.substr(0, last_slash + 1);
            file_directory[parent_dir].insert(name.substr(last_slash + 1));
        } else { // Regular file
            char *buffer = new char[size + 1];
            file.read(buffer, size);
            buffer[size] = '\0';
            file_content[name] = buffer;
            size_t last_slash = name.find_last_of('/', name.size() - 2);
            string parent_dir = (last_slash == string::npos) ? "/" : name.substr(0, last_slash + 1);
            file_directory[parent_dir].insert(name.substr(last_slash + 1));
            //cout<<name<<":\n";
            //cout<<buffer<<"\n";
        }

        file_attribute[name] = st;
        file.ignore((512 - (size % 512)) % 512);
    }
    /*for (const auto& dir : file_directory) {
        cout << "Directory: " << dir.first << endl;
        for (const auto& file : dir.second) {
            cout << "  - " << file << endl;
        }
    }
    for (const auto& dir : symlink_targets) {
        cout << "softlink: "<<dir.first<<" "<<dir.second<< endl;
    }*/
    memset(&op, 0, sizeof(op));
    op.getattr = my_getattr;
    op.readdir = my_readdir;
    op.read = my_read;
    op.readlink = my_readlink;
    return fuse_main(argc, argv, &op, NULL);
}
/*./hw6.out -f tarfs
Directory: /
  - dir/
  - dir1/
  - largefile/
Directory: dir/
Directory: dir1/
  - dir2/
Directory: dir1/dir2/
Directory: largefile/*/
