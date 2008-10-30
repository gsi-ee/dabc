package xgui;
import java.io.*;
public class xConvert
{
  public final static int swap(int i, int e)
  {
    if(e != 1) i = (((i & 0x000000ff) <<  24) |
		    ((i & 0x0000ff00) <<   8) |
		    ((i & 0x00ff0000) >>   8) |
		    ((i & 0xff000000) >>> 24));
    return i;
  }
  public final static int iswap(DataInputStream in, int e)
  {
    int i=0;
    try{i = in.readInt();}
    catch(IOException e1){System.err.println(e1);}
    if(e != 1) i = (((i & 0x000000ff) <<  24) |
		    ((i & 0x0000ff00) <<   8) |
		    ((i & 0x00ff0000) >>   8) |
		    ((i & 0xff000000) >>> 24));
    return i;
  }
  public final static String istr(DataInputStream in, int e)
  {
    int i=0;
    try{i = in.readInt();}
    catch(IOException e1){System.err.println(e1);}
    if(e != 1) i = (((i & 0x000000ff) <<  24) |
		    ((i & 0x0000ff00) <<   8) |
		    ((i & 0x00ff0000) >>   8) |
		    ((i & 0xff000000) >>> 24));
    return String.valueOf(i);
  }
  public final static String str(DataInputStream in, byte b[])
  {
    try{in.readFully(b);}
    catch(IOException e1){System.err.println(e1);}
    int i=0;
    while(b[i]>=32)i++;
    return new String(b,0,i);
  }
}
