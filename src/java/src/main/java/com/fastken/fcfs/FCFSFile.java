package com.fastken.fcfs;

import java.util.List;

public class FCFSFile {

    private static native void doInit();

    static {
        doInit();
    }

    public native void close();
    public native void sync();
    public native void datasync();
    public native void write(byte[] bs, int off, int len);
    public native int read(byte[] bs, int off, int len);
    public native long lseek(long offset, int whence);
    public native void allocate(int mode, long offset, long length);
    public native void truncate(long length);
    public native FCFSFileStat stat();
    public native boolean lock(long position, long length,
            boolean shared, boolean blocked);
    public native void unlock(long position, long length);
    public native void utimes(long atime, long mtime);
    public native void chown(int owner, int group);
    public native void chmod(int mode);
    public native void setxattr(String name, byte[] bs,
            int off, int len, int flags);
    public native void removexattr(String name);
    public native byte[] getxattr(String name);
    public native List<String> listxattr();
    public native void chdir();
    public native FCFSVFSStat statvfs();

    private int fd;

    public FCFSFile(int fd) {
        this.fd = fd;
    }

    public void setFD(int fd) {
        this.fd = fd;
    }

    public int getFD() {
        return this.fd;
    }

    public void write(byte[] bs) {
        this.write(bs, 0, bs.length);
    }

    public int read(byte[] bs) {
        return this.read(bs, 0, bs.length);
    }

    public boolean lock(long position, long length, boolean shared)
    {
        final boolean blocked = true;
        return this.lock(position, length, shared, blocked);
    }

    public boolean tryLock(long position, long length, boolean shared)
    {
        final boolean blocked = false;
        return this.lock(position, length, shared, blocked);
    }

     public void setxattr(String name, byte[] bs, int flags)
     {
         this.setxattr(name, bs, 0, bs.length, flags);
     }
}
