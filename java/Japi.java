package dujeonglee.networkcoding;

public class Japi
{
    static 
    {
        System.loadLibrary("jncsocket");
    }

    public native long initSocket(final String local_port, final int rxTimeout, final int txTimeout);
    public native boolean connect(final long handle, final String ip, final String port, final int timeout, final int transmissionMode, final int blockSize, final int retransmissionRedundancy);
    public native void disconnect(final long handle, final String ip, final String port);
    public native boolean send(final long handle, final String ip, final String port, byte[] buff, final int size);
    public native boolean flush(final long handle, final String ip, final String port);
    public native void waitUntilTxIsCompleted(final long handle, final String ip, final String port);
    public native int receive(final long handle, byte[] buffer, int bufferSize, String[] senderInfo, int senderInfoSize, int timeout);
    public native void freeSocket(final long handle);
} 
