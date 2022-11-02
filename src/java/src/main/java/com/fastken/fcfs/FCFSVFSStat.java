package com.fastken.fcfs;
  
public class FCFSVFSStat {
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


    private Stat space;
    private Stat inode;

    static {
        doInit();
    }

    private static native void doInit();

    public FCFSVFSStat(long spaceTotal, long spaceAvail, long spaceUsed,
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
