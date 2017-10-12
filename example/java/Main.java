import dujeonglee.networkcoding.Japi;

public class Main
{
    public static void main (String[] args)
    {
        System.out.println("Args : "+args.length);
        Japi ncsocket = new Japi();
        final long handle = ncsocket.InitSocket(args[0], 500, 500);
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