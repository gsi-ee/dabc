package xgui;
import java.util.*;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.Cursor;
import java.io.*;
import org.w3c.dom.*;
import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JDesktopPane;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;

/**
 * Singleton and registry.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xSet
{
private static String  DimDns   = new String(System.getenv("DIM_DNS_NODE"));
private static String  UserName = new String(System.getenv("USER"));
private static String  GuiNode  = new String(System.getenv("HOST"));
private static String  Access   = new String(xCrypt.crypt("x1",""));
private static Color red=      new Color(1.0f, 0.0f, 0.0f);
private static Color redL=     new Color(1.0f, 0.7f, 0.5f);
private static Color redD=     new Color(0.5f, 0.0f, 0.0f);
private static Color green=    new Color(0.0f, 1.0f, 0.0f);
private static Color greenL=   new Color(0.5f, 1.0f, 0.5f);
private static Color greenD=   new Color(0.0f, 0.5f, 0.0f);
private static Color blue=     new Color(0.0f, 0.0f, 1.0f);
private static Color blueL=    new Color(0.5f, 0.8f, 1.0f);
private static Color blueD=    new Color(0.0f, 0.0f, 0.5f);
private static Color yellow=   new Color(1.0f, 1.0f, 0.0f);
private static Color yellowL=  new Color(1.0f, 1.0f, 0.5f);
private static Color yellowD=  new Color(0.5f, 0.5f, 0.0f);
private static Color magenta=  new Color(1.0f, 0.0f, 1.0f);
private static Color magentaL= new Color(1.0f, 0.5f, 1.0f);
private static Color magentaD= new Color(0.5f, 0.0f, 0.5f);
private static Color cyan=     new Color(0.0f, 1.0f, 1.0f);
private static Color cyanL=    new Color(0.5f, 1.0f, 1.0f);
private static Color cyanD=    new Color(0.0f, 0.5f, 0.5f);
private static Color black=    new Color(0.0f, 0.0f, 0.0f);
private static Color gray=     new Color(0.5f, 0.5f, 0.5f);
private static Color grayL=    new Color(0.8f, 0.8f, 0.8f);
private static Color grayD=    new Color(0.3f, 0.3f, 0.3f);
private static Color white=    new Color(1.0f, 1.0f, 1.0f);
private static Color colorBack=new Color(0.0f, 0.0f, 0.3f);
private static Color colorText=new Color(1.0f, 1.0f, 1.0f);
private static Vector<xLayout> layouts = new Vector<xLayout>();
private static Vector<Object> objects = new Vector<Object>();
private static NodeList records=null;
private static JDesktopPane desktop=null;
private static JFrame guiframe=null;
private static int[] ParTableWidth;
private static StringBuffer str;
private static boolean OK;
private static String msg;

/**
 * Singleton
 */
public xSet(){
}
/**
 * Set background color. Userpanel may set this color in constructor.
 * Than all panels inherit this color.
 * @param back Color for background
 */
public final static void setColorBack(Color back){colorBack=back;}
public final static void setColorText(Color text){colorText=text;}
public final static Color getColorBack(){return colorBack;}
public final static Color getColorText(){return colorText;}
public final static Color red(){return red;}  
public final static Color redL(){return redL;}  
public final static Color redD(){return redD;} 
public final static Color green(){return green;} 
public final static Color greenL(){return greenL;}
public final static Color greenD(){return greenD;} 
public final static Color blue(){return blue;}   
public final static Color blueL(){return blueL;} 
public final static Color blueD(){return blueD;} 
public final static Color yellow(){return yellow;}  
public final static Color yellowL(){return yellowL;} 
public final static Color yellowD(){return yellowD;}  
public final static Color magenta(){return magenta;} 
public final static Color magentaL(){return magentaL;}
public final static Color magentaD(){return magentaD;} 
public final static Color cyan(){return cyan;}  
public final static Color cyanL(){return cyanL;}  
public final static Color cyanD(){return cyanD;} 
public final static Color black(){return black;} 
public final static Color gray(){return gray;} 
public final static Color grayL(){return grayL;} 
public final static Color grayD(){return grayD;}  
public final static Color white(){return white;}  

public final static void setRecordXml(NodeList list){
records=list;
}
public static final NodeList getRecordXml(){return records;}
/**
 * Adds object to repository (only one per class!).
 * @param object Object to add.
 * @return Null if an object of this class already exists.
 */
public final static Object addObject(Object object){
if(getObject(object.getClass().getName()) == null){
    objects.add(object);
    return object;
} else return null;
}
/**
 * @param classname Only one object per class!
 * @return Object reference or null.
 */
public final static Object getObject(String classname){
for(int i=0;i < objects.size();i++)
    if(objects.elementAt(i).getClass().getName().equals(classname)) return objects.elementAt(i);
return null;
}
/**
 * Layouts keep position and size of windows.
 * Most layouts are created in xDesktop, but user can add his own.
 * All layouts are stored with the save layout button.
 * On startup of the GUI layouts are retrieved. User can get his layouts
 * by getLayout method. 
 * @param name Name of layout.
 * @return New layout or null, if one already exists with same name.
 */
public final static xLayout createLayout(String name){
for(int i=0;i < layouts.size();i++)
    if(layouts.elementAt(i).getName().equals(name)) return null;
// not found, can add
xLayout lo=new xLayout(name);
lo.set(new Point(0,0), new Dimension(100,100),0,false);
layouts.add(lo);
return lo;
}
/**
 * See also createLayout.
 * @param name Name of layout.
 * @param pos Position
 * @param size Size of window
 * @param columns Columns in the panels of states, meters, and histograms.
 * @param visible True if component should be show up on startup.
 * @return New layout or null, if one already exists with same name.
 */
public final static xLayout createLayout(String name, Point pos, Dimension size, int columns, boolean visible){
for(int i=0;i < layouts.size();i++) if(layouts.elementAt(i).getName().equals(name)) return null;
// not found, can add
xLayout lo=new xLayout(name);
lo.set(pos,size,columns,visible);
layouts.add(lo);
return lo;
}
/**
 * See also createLayout.
 * @param name Name of layout.
 * @param pos Position
 * @param size Size of window
 * @param columns Columns in the panels of states, meters, and histograms.
 * @param visible True if component should be show up on startup.
 * @return True if layout exists, otherwise null.
 * @see xLayout 
 */
public final static boolean setLayout(String name, Point pos, Dimension size, int columns, boolean visible){
for(int i=0;i < layouts.size();i++){
    if(layouts.elementAt(i).getName().equals(name)){
    layouts.elementAt(i).set(pos,size,columns,visible);
    return true;
}}
return false;
}
/**
 * See also createLayout.
 * @param name Name of layout.
 * @return Layout or null (not found).
 */
public final static xLayout getLayout(String name){
for(int i=0;i < layouts.size();i++) if(layouts.elementAt(i).getName().equals(name)) return layouts.elementAt(i);
// not found
return null;
}
/**
 * See also createLayout.
 * @return Layouts.
 */
public final static Vector<xLayout> getLayouts(){return layouts;}
/**
 * Does not work as expected!
 */
protected final static void setWaitCursor(){
    JInternalFrame[] fr=desktop.getAllFrames();
    guiframe.setCursor(new Cursor(Cursor.WAIT_CURSOR));
    for(int i=0;i<fr.length;i++)fr[i].setCursor(new Cursor(Cursor.WAIT_CURSOR));
} 
/**
 * Does not work as expected!
 */
protected final static void setDefaultCursor(){
    JInternalFrame[] fr=desktop.getAllFrames();
    guiframe.setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
    for(int i=0;i<fr.length;i++)fr[i].setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
} 
// setup the geometry of the windows
protected final static void setParTableWidth(int[] w){ParTableWidth=w;}
// store desktop
protected final static void setDesktop(JFrame gui, JDesktopPane dp){guiframe=gui; desktop=dp;}
/**
 * @param success Value can be retrieved by isSuccess().
 */
public final static void setSuccess(boolean success){OK=success;}
/**
 * @param message Value can be retrieved by getMessage().
 */
public final static void setMessage(String message){msg=new String(message);}
/**
 * @return Value previously set by setSuccess(...).
 */
public final static boolean isSuccess(){return OK;}
/**
 * @return Value previously set by setMessage(...).
 */
public final static String getMessage(){return msg;}
/**
 * @return Widths of the parameter table columns.
 */
protected final static int[] getParTableWidth(){return ParTableWidth;}
/**
 * @return DIM name server node
 */
public final static String getDimDns(){return DimDns;}
/**
 * @return User name.
 */
public final static String getUserName(){return UserName;}
/**
 * @return Host name.
 */
public final static String getGuiNode(){return GuiNode;}
/**
 * Sometimes one needs the top pane.
 * @return Desktop pane.
 * @see xHisto
 * @see xMeter
 * @see xPanelPrompt
 */
protected final static JDesktopPane getDesktop(){return desktop;}
/**
 * Store encrypted password (crypt).
 * @param access Password as retrieved from JPasswordField.
 */
protected final static void setAccess(char[] access){
Access = new String(xCrypt.crypt("x1",new String(access)));
}
/**
 * @return Encrypted password.
 */
public final static String getAccess(){return Access;}
/**
 * Dimension adder for convenience.
 * @param d1 First dimension
 * @param d2 Second dimension
 * @return New dimension with added w and h.
 */
public final static Dimension addDimension(Dimension d1, Dimension d2){
return new Dimension((int)(d1.getWidth()+d2.getWidth()),(int)(d1.getHeight()+d2.getHeight()));
}
/**
 * To read images from jar file, we need this ugly stuff.
 * @param file Icon file name.
 * @return The icon or null (not found).
 */
public final static ImageIcon getIcon(String file){
    InputStream is = ClassLoader.getSystemResourceAsStream(file);
    try{
    return new ImageIcon(ImageIO.read(is));
    } catch(IOException e){}
    return null;
}
/**
 * Set layout values from XML element. Called by xSaveRestore.restoreLayouts.
 * @param layout Element of WindowLayout elements.
 * @see xSaveRestore
 */
protected final static void setWindowLayout(Element layout){
String[] sx;
xLayout lo=createLayout(layout.getTagName());
if(lo == null)lo=getLayout(layout.getTagName());
sx=layout.getAttribute("shape").toString().split(",");
lo.set(new Point(Integer.parseInt(sx[0]),Integer.parseInt(sx[1])),
    new Dimension(Integer.parseInt(sx[2]),Integer.parseInt(sx[3])),
    Integer.parseInt(layout.getAttribute("columns").toString()),
    Boolean.parseBoolean(layout.getAttribute("show").toString()));
}
/**
 * Set table values from XML element. Called by xSaveRestore.restoreLayouts.
 * @param name Name of element of TableLayout ("Parameter").
 * @param layout Element of TableLayout.
 * @see xSaveRestore
 */
protected final static void setTableLayout(String name, Element layout){
String[] sx;
Element el;
el=(Element)layout.getElementsByTagName(name).item(0);
sx=el.getAttribute("width").toString().split(",");
for(int i=0;i<sx.length;i++)ParTableWidth[i]=Integer.parseInt(sx[i]);
}
}
