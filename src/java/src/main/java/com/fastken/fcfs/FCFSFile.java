package com.fastken.fcfs;

import java.util.Map;
import java.nio.file.FileSystemException;

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
    public native void lock(long position, long length, boolean shared, boolean blocked);
    public native void unlock(long position, long length);
    public native void utimes(long[] times);
    public native void chown(int owner, int group);
    public native void chmod(int mode);
    public native void setxattr(String name, byte[] bs, int off, int len, int flags);
    public native void removexattr(String name);
    public native byte[] getxattr(String name);
    public native Map<String, byte[]> listxattr();
    public native void chdir();
    public native FCFSVFSStat statvfs();

    private int fd;

    public FCFSFile(int fd) {
        this.fd = fd;
    }

    public void setFD(int fd) {
        this.fd = fd;
    }

    public long getFD() {
        return this.fd;
    }

    public void write(byte[] bs) {
        this.write(bs, 0, bs.length);
    }

    public int read(byte[] bs) {
        return this.read(bs, 0, bs.length);
    }
}
