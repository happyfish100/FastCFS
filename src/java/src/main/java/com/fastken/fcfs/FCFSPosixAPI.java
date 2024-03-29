package com.fastken.fcfs;
  
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.io.File;
import java.io.UnsupportedEncodingException;
import java.nio.file.FileSystemException;

public class FCFSPosixAPI {
    private static HashMap<String, FCFSPosixAPI> instances = new HashMap<String, FCFSPosixAPI>();
    private static final String charset = "UTF-8";
    private static String libraryFilename = null;

    private static native void doInit();

    private native void init(String ns, String configFilename);
    private native void destroy();

    public native FCFSDirectory opendir(String path);
    public native FCFSFile open(String path, int flags, int mode);

    public native String getcwd();
    public native void truncate(String path, long length);
    public native FCFSFileStat stat(String path, boolean followlink);
    public native void link(String path1, String path2);
    public native void symlink(String link, String path);
    public native String readlink(String path);
    public native void mknod(String path, int mode, int dev);
    public native void mkfifo(String path, int mode);
    public native void access(String path, int mode);
    public native boolean exists(String path);
    public native void utimes(String path, long atime, long mtime);
    public native void unlink(String path);
    public native void rename(String path1, String path2);
    public native void mkdir(String path, int mode);
    public native void rmdir(String path);
    public native void chown(String path, int owner, int group, boolean followlink);
    public native void chmod(String path, int mode);
    public native FCFSVFSStat statvfs(String path);
    public native void chdir(String path);
    public native void setxattr(String path, String name, byte[] b,
            int off, int len, int flags, boolean followlink);
    public native void removexattr(String path, String name, boolean followlink);
    public native byte[] getxattr(String path, String name, boolean followlink);
    public native List<String> listxattr(String path, boolean followlink);

    private long handler;

    public static String getLibraryFilename() {
        return libraryFilename;
    }

    /**
      * set libfcfsjni.so filename to load
      * @param filename the full filename
      * @return none
      */
    public static void setLibraryFilename(String filename) {
        if (libraryFilename == null) {
            System.load(filename);  //load the library
            libraryFilename = filename;
            doInit();
        } else if (libraryFilename.equals(filename)) {
            System.err.println("[WARNING] library " + libraryFilename + " already loaded");
        } else {
            throw new RuntimeException("library " + libraryFilename
                    + " already loaded, can't change to " + filename);
        }
    }

    // private for singleton
    private FCFSPosixAPI(String ns, String configFilename) {
        init(ns, configFilename);
    }

    public void setHandler(long handler) {
        this.handler = handler;
    }

    public long getHandler() {
        return this.handler;
    }

    protected void finalize() throws Throwable {
        super.finalize();
        this.close();
    }

    private void close() {
        System.out.println("close");
        destroy();
    }


    /**
      * get FCFSPosixAPI instance
      * @param ns the namespace / poolname
      * @param configFilename the config filename such as /etc/fastcfs/fcfs/fuse.conf
      * @return FCFSPosixAPI object
     */
    public synchronized static FCFSPosixAPI getInstance(String ns, String configFilename) {
        String key = ns + "@" + configFilename;
        FCFSPosixAPI obj = instances.get(key);
        if (obj == null) {
            obj = new FCFSPosixAPI(ns, configFilename);
            instances.put(key, obj);
        }

        return obj;
    }

    /**
      * clear FCFSPosixAPI instances
      * @return none
     */
    public synchronized static void clearInstances() {
        instances.clear();
    }

    /**
      * open file for read
      * @param path the filename to open
      * @return FCFSFile instance
     */
    public FCFSFile open(String path)
    {
        final int mode = 0;
        return this.open(path, FCFSConstants.O_RDONLY, mode);
    }

    public void setxattr(String path, String name, byte[] b, int flags) {
        final boolean followlink = true;
        this.setxattr(path, name, b, 0, b.length, flags, followlink);
    }

    public void setxattr(String path, String name, byte[] b) {
        final int flags = 0;
        final boolean followlink = true;
        this.setxattr(path, name, b, 0, b.length, flags, followlink);
    }

    public void lsetxattr(String path, String name, byte[] b, int flags) {
        final boolean followlink = false;
        this.setxattr(path, name, b, 0, b.length, flags, followlink);
    }

    public void lsetxattr(String path, String name, byte[] b) {
        final int flags = 0;
        final boolean followlink = false;
        this.setxattr(path, name, b, 0, b.length, flags, followlink);
    }

    public FCFSFileStat stat(String path) {
        final boolean followlink = true;
        return this.stat(path, followlink);
    }

    public FCFSFileStat lstat(String path) {
        final boolean followlink = false;
        return this.stat(path, followlink);
    }

    public static void main(String[] args) throws Exception {
        final String ns = "fs";
        final String configFilename = "/etc/fastcfs/fcfs/fuse.conf";
        final boolean followlink = false;
        String path;
        FCFSPosixAPI papi;

        File f = new File("/usr/lib/libfcfsjni.so");
        if (f.exists()) {
            FCFSPosixAPI.setLibraryFilename(f.getAbsolutePath());
        } else {
            FCFSPosixAPI.setLibraryFilename("/usr/local/lib/libfcfsjni.so");
        }

        papi = FCFSPosixAPI.getInstance(ns, configFilename);

        path = "/opt/fastcfs/fuse/";
        String filename = path + "test.txt";
        String filename1 = filename;
        String filename2 = path + "test2.txt";

        FCFSDirectory dir = papi.opendir(path);
        FCFSDirectory.Entry dirent;
        FCFSFile file;

        System.out.println("cwd: " + papi.getcwd());
        while ((dirent=dir.next()) != null) {
            //System.out.println("inode: " + dirent.getInode() + ", name: " + dirent.getName());
        }
        dir.close();

        try {
            papi.unlink(filename1);
        } catch(Exception ex) {
        }

        if (!papi.exists(filename1)) {
            file = papi.open(filename1, FCFSConstants.O_WRONLY |
                    FCFSConstants.O_CREAT, 0755);
            file.write(new String("hello world!").getBytes(charset));
            file.close();

            System.out.println(filename1 + " created");
        }

        try {
            papi.unlink(filename2);
        } catch(Exception ex) {
        }
        papi.symlink("./test.txt", filename2);
        //papi.symlink(filename1, filename2);
        System.out.println("readlink: " + papi.readlink(filename2));

        byte[] bs = new byte[1024];
        file = papi.open(filename2);
        int length = file.read(bs);
        file.close();
        System.out.println("content: " + new String(bs, 0, length, charset));

        System.out.println("fstat: " + papi.stat(filename));
        papi.truncate(filename1, 4 * 1024);
        System.out.println("fstat: " + papi.stat(filename2));

        //System.out.println("statvfs: " + papi.statvfs(path));

        papi.setxattr(path, "myxattr", new String("myvalue").getBytes(charset));

        List<String> list;
        list = papi.listxattr(path, followlink);
        for (String name : list) {
            System.out.println("name: " + name + ", value: "
                    + new String(papi.getxattr(path, name, followlink), charset));
        }


        file = papi.open(filename);
        System.out.println("fstat: " + file.stat());
        //System.out.println("fstatvfs: " + file.statvfs());

        list = file.listxattr();
        for (String name : list) {
            System.out.println("name: " + name + ", value: "
                    + new String(file.getxattr(name), charset));
        }

        file.close();

        papi.close();
    }
}
