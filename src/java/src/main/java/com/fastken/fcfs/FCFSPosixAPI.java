package com.fastken.fcfs;
  
import java.util.List;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.io.UnsupportedEncodingException;

public class FCFSPosixAPI {
    public static class Buffer {
        private byte[] buff;
        private int offset;  //offset of the buff
        private int length;  //data length

        public Buffer(byte[] buff, int offset, int length) {
            this.buff = buff;
            this.offset = offset;
            this.length = length;
        }

        public Buffer(byte[] buff) {
            this.buff = buff;
            this.offset = 0;
            this.length = buff.length;
        }

        public void setLength(int length) {
            this.length = length;
        }

        public byte[] getBuff() {
            return this.buff;
        }

        public int getOffset() {
            return this.offset;
        }

        public int getLength() {
            return this.length;
        }
    }

    public static class DIR {
        private long handler;

        public DIR(long handler) {
            this.handler = handler;
        }

        public long getHandler() {
            return this.handler;
        }
    }

    public static class DIREntry {
        private long inode;
        private String name;

        public DIREntry(long inode, String name) {
            this.inode = inode;
            this.name = name;
        }

        public long getInode() {
            return this.inode;
        }

        public String getName() {
            return this.name;
        }
    }

    public static class FileStat {
        private long inode;  /* Inode number */
        private int mode;    /* File type and mode */
        private int links;   /* Number of hard links */
        private int uid;     /* User ID of owner */
        private int gid;     /* Group ID of owner */
        private int rdev;    /* Device ID (if special file) */
        private long size;   /* Total size, in bytes */
        private long atime;  /* Time of last access */
        private long mtime;  /* Time of last modification */
        private long ctime;  /* Time of last status change */

        public FileStat(long inode, int mode, int links, int uid, int gid,
                int rdev, long size, long atime, long mtime, long ctime)
        {
            this.inode = inode;
            this.mode = mode;
            this.links = links;
            this.uid = uid;
            this.gid = gid;
            this.rdev = rdev;
            this.size = size;
            this.atime = atime;
            this.mtime = mtime;
            this.ctime = ctime;
        }

        public long getInode() {
            return this.inode;
        }

        public long getMode() {
            return this.mode;
        }

        public long getLinks() {
            return this.links;
        }

        public long getUid() {
            return this.uid;
        }

        public long getGid() {
            return this.gid;
        }

        public long getRdev() {
            return this.rdev;
        }

        public long getSize() {
            return this.size;
        }

        public long getAtime() {
            return this.atime;
        }

        public long getMtime() {
            return this.mtime;
        }

        public long getCtime() {
            return this.ctime;
        }
    }

    public static class Stat {
        private long total;
        private long avail;
        private long used;

        public Stat(long total, long avail, long used) {
            this.total = total;
            this.avail = avail;
            this.used = used;
        }

        public long getTotal() {
            return this.total;
        }

        public long getAvail() {
            return this.avail;
        }

        public long getUsed() {
            return this.used;
        }
    }

    public static class VFSStat {
        private Stat space;
        private Stat inode;

        public VFSStat(long spaceTotal, long spaceAvail, long spaceUsed,
                long inodeTotal, long inodeAvail, long inodeUsed)
        {
            this.space = new Stat(spaceTotal, spaceAvail, spaceUsed);
            this.inode = new Stat(inodeTotal, inodeAvail, inodeUsed);
        }

        public Stat getSpaceStat() {
            return this.space;
        }

        public Stat getInodeStat() {
            return this.inode;
        }
    }

    private static HashMap<String, FCFSPosixAPI> instances = new HashMap<String, FCFSPosixAPI>();
    private static String charset = "UTF-8";
    private static String libraryFilename = null;

    private long handler;

    private native long doInit(String configFilename);
    private native void doDestroy(long handler);

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
        } else if (libraryFilename.equals(filename)) {
            System.err.println("[WARNING] library " + libraryFilename + " already loaded");
        } else {
            throw new RuntimeException("library " + libraryFilename
                    + " already loaded, can't change to " + filename);
        }
    }

    public static String getCharset() {
        return charset;
    }

    public static void setCharset(String value) {
        charset = value;
    }

    // private for singleton
    private FCFSPosixAPI(String configFilename) {
        this.handler = doInit(configFilename);
    }

    protected void finalize() throws Throwable {
        super.finalize();
        this.close();
    }

    private void close() {
        System.out.println("close");
        doDestroy(this.handler);
    }


    /**
      * get FCFSPosixAPI instance
      * @param configFilename the config filename such as /usr/local/etc/libshmcache.conf
      * @return FCFSPosixAPI object
     */
    public synchronized static FCFSPosixAPI getInstance(String configFilename) {
        FCFSPosixAPI obj = instances.get(configFilename);
        if (obj == null) {
            obj = new FCFSPosixAPI(configFilename);
            instances.put(configFilename, obj);
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

}
