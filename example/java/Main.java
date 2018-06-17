import dujeonglee.networkcoding.Japi;

public class Main
{
    public final static byte BYEBYE = 0;
    public static void main (String[] args)
    {
        if(args.length == 1)
        {
            System.out.println("Rx");
            byte[] buffer = new byte[1500];
            String[] info = new String[2];
            final Japi ncsocket = new Japi();
            final long handle = ncsocket.initSocket(args[0], 500, 500);
            
            do {
                int ret = ncsocket.receive(handle, buffer, buffer.length, info, info.length, 500);
                if(ret == 1 && buffer[0] == BYEBYE)
                {
                    System.out.println("Bye.");
                    break;
                }
                else if(ret > 1)
                {
                    System.out.println("Receive "+ret+" bytes: "+new String(buffer, 0, ret)+".");
                }
                else
                {
                    System.out.println("Receive timeout.");
                }
            } while (true);
            ncsocket.freeSocket(handle);
        }
        else if(args.length == 3)
        {
            System.out.println("Tx");
            byte[] buffer = new byte[1500];
            buffer[0] = 'H';
            buffer[1] = 'e';
            buffer[2] = 'l';
            buffer[3] = 'l';
            buffer[4] = 'o';
            buffer[5] = 0;
            String[] info = new String[2];
            final Japi ncsocket = new Japi();
            final long handle = ncsocket.initSocket(args[0], 500, 500);
            while(false==ncsocket.connect(handle, args[1], args[2], 1000, 0, 8, 0))
            {
                System.out.println("Connecting to "+args[1]+":"+args[2]+".");
            }
            
            for(int i = 0 ; i < 100 ; i++)
            {
                buffer[5] = (byte)i;
                if(true == ncsocket.send(handle, args[1], args[2], buffer, 5))
                {
                    System.out.println("Sent data.");
                }
            }
            buffer[0] = BYEBYE;
            ncsocket.send(handle, args[1], args[2], buffer, 1);
            ncsocket.waitUntilTxIsCompleted(handle, args[1], args[2]);
            ncsocket.freeSocket(handle);
        }
        else
        {
            System.out.println("1. Rx : # java Main port.");
            System.out.println("2. Tx : # java Main port remoteip remoteport.");
        }
    }
}
