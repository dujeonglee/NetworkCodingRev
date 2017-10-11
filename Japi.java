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
    public native void FreeSocket(final long handle);

    public void Callback(byte[] buffer, final String ip, final String port)
    {
        System.out.println("RECV!!");
    }

    public static void main (String[] args)
    {
        System.out.println("Args : "+args.length);
        Japi ncsocket = new Japi();
        final long handle = ncsocket.InitSocket(args[0], 500, 500);
        System.out.println("Handle : "+handle);
        while(false == ncsocket.Connect(handle, args[1], args[2], 1000, 0, 8, 0))
        {
            System.out.println("Connecting...");
        }
        System.out.println("Connected");
        byte[] array = new byte[5];
        array[0] = 'H';
        array[1] = 'e';
        array[2] = 'l';
        array[3] = 'l';
        array[4] = 'o';
        if(ncsocket.Send(handle, args[1], args[2], array, 5) == true)
        {
            System.out.println("Sent");
        }
        ncsocket.WaitUntilTxIsCompleted(handle, args[1], args[2]);
        System.out.println("Flush");
        while(true);
    }
} 
