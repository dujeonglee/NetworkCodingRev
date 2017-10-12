package dujeonglee.networkcoding;

public class Japi
{
    static 
    {
        System.loadLibrary("jncsocket");
    }

    public native long InitSocket(final String local_port, final int rxTimeout, final int txTimeout);
    public native boolean Connect(final long handle, final String ip, final String port, final int timeout, final int transmissionMode, final int blockSize, final int retransmissionRedundancy);
    public native void Disconnect(final long handle, final String ip, final String port);
    public native boolean Send(final long handle, final String ip, final String port, byte[] buff, final int size);
    public native boolean Flush(final long handle, final String ip, final String port);
    public native void WaitUntilTxIsCompleted(final long handle, final String ip, final String port);
    public native int Receive(final long handle, byte[] buffer, String[] senderInfo);
    public native void FreeSocket(final long handle);
} 
