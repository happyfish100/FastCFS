package com.fastken.fcfs;

public class FCFSConstants {
    //for access mode
    public static final int F_OK = 0;  //check file existence
    public static final int X_OK = 1;  //execute permission
    public static final int W_OK = 2;  //write permission
    public static final int R_OK = 4;  //read permission


    //for setxattr flags
    /* perform a pure create operation, which fails if the named
       attribute exists already. */
    public static final int XATTR_CREATE = (1 << 0);

    /*Perform a pure replace operation, which fails if the named
      attribute does not already exist. */
    public static final int XATTR_REPLACE = (1 << 1);


    //for lseek whence
    /* The file offset is set to offset bytes. */
    public static final int SEEK_SET = 0;

    /* The file offset is set to its current location plus offset bytes. */
    public static final int SEEK_CUR = 1;

    /* The file offset is set to the size of the file plus offset bytes. */
    public static final int SEEK_END = 2;


    //for file open flags
    public static final int O_RDONLY   = 00;
    public static final int O_WRONLY   = 01;
    public static final int O_RDWR     = 02;
    public static final int O_CREAT    = 0100;
    public static final int O_EXCL     = 0200;
    public static final int O_TRUNC    = 01000;
    public static final int O_APPEND   = 02000;
    public static final int O_NOFOLLOW = 0400000;
    public static final int O_CLOEXEC  = 02000000;
}
