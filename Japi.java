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
        while(true)
        {
            byte[] buffer = new byte[1500];
            String[] info = new String[2];
            String ip = new String("");
            String port = new String("");
            int ret = ncsocket.Receive(handle, buffer, info);
            if(ret > 0)
            {
                System.out.println(ret+" "+info[0]+" "+info[1]);
                System.out.println((char)buffer[0]);
                System.out.println((char)buffer[1]);
                System.out.println((char)buffer[2]);
                System.out.println((char)buffer[3]);
                System.out.println((char)buffer[4]);
            }
            try{
                Thread.sleep(1000);
            }catch(Exception e){
                System.out.println(e.getMessage());
            }
        }
    }
} 
