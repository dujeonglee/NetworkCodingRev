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
            final long handle = ncsocket.InitSocket(args[0], 500, 500);
            
            do {
                int ret = ncsocket.Receive(handle, buffer, buffer.length, info, info.length, 500);
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
            ncsocket.FreeSocket(handle);
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
            final long handle = ncsocket.InitSocket(args[0], 500, 500);
            while(false==ncsocket.Connect(handle, args[1], args[2], 1000, 0, 8, 0))
            {
                System.out.println("Connecting to "+args[1]+":"+args[2]+".");
            }
            
            for(int i = 0 ; i < 100 ; i++)
            {
                buffer[5] = (byte)i;
                if(true == ncsocket.Send(handle, args[1], args[2], buffer, 5))
                {
                    System.out.println("Sent data.");
                }
            }
            buffer[0] = BYEBYE;
            ncsocket.Send(handle, args[1], args[2], buffer, 1);
            ncsocket.WaitUntilTxIsCompleted(handle, args[1], args[2]);
            ncsocket.FreeSocket(handle);
        }
        else
        {
            System.out.println("1. Rx : # java Main port.");
            System.out.println("2. Tx : # java Main port remoteip remoteport.");
        }
    }
}
