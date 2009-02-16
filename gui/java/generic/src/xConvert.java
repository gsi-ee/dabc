/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
package xgui;
import java.io.*;
/**
 * Swapping function.
 * @author Hans G. Essel
 * @version 1.0
 */
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
