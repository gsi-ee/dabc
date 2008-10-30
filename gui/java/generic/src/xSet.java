package xgui;
import java.util.*;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.Cursor;
import java.io.*;
import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import javax.imageio.ImageIO;
import javax.swing.ImageIcon;
import javax.swing.JDesktopPane;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;

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
private static JDesktopPane desktop=null;
private static JFrame guiframe=null;
private static int[] ParTableWidth;
private static StringBuffer str;
private static boolean OK;
private static String msg;
/**
 * Singleton.
 * @param head name of parameter displayed
 */
public xSet(){
}

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

public final static void addObject(Object object){
objects.add(object);
}
public final static Object getObject(String classname){
for(int i=0;i < objects.size();i++)
    if(objects.elementAt(i).getClass().getName().equals(classname)) return objects.elementAt(i);
return null;
}

public final static xLayout createLayout(String name){
for(int i=0;i < layouts.size();i++)
    if(layouts.elementAt(i).getName().equals(name)) return null;
// not found, can add
xLayout lo=new xLayout(name);
lo.set(new Point(0,0), new Dimension(100,100),0,false);
layouts.add(lo);
return lo;
}

public final static xLayout createLayout(String name, Point pos, Dimension size, int columns, boolean show){
for(int i=0;i < layouts.size();i++) if(layouts.elementAt(i).getName().equals(name)) return null;
// not found, can add
xLayout lo=new xLayout(name);
lo.set(pos,size,columns,show);
layouts.add(lo);
return lo;
}

public final static boolean setLayout(String name, Point pos, Dimension size, int columns, boolean show){
for(int i=0;i < layouts.size();i++){
    if(layouts.elementAt(i).getName().equals(name)){
    layouts.elementAt(i).set(pos,size,columns,show);
    return true;
}}
return false;
}

public final static xLayout getLayout(String name){
for(int i=0;i < layouts.size();i++) if(layouts.elementAt(i).getName().equals(name)) return layouts.elementAt(i);
// not found
return null;
}
public final static JInternalFrame getDesktopFrame(String title){
    JInternalFrame[] fr=desktop.getAllFrames();
    for(int i=0;i<fr.length;i++){
        if(fr[i].getTitle().equals(title)) return fr[i];
    }
    return null;
} 
public final static void setWaitCursor(){
    JInternalFrame[] fr=desktop.getAllFrames();
    guiframe.setCursor(new Cursor(Cursor.WAIT_CURSOR));
    for(int i=0;i<fr.length;i++)fr[i].setCursor(new Cursor(Cursor.WAIT_CURSOR));
} 
public final static void setDefaultCursor(){
    JInternalFrame[] fr=desktop.getAllFrames();
    guiframe.setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
    for(int i=0;i<fr.length;i++)fr[i].setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
} 
// setup the geometry of the windows
public final static void setParTableWidth(int[] w){ParTableWidth=w;}
// store desktop
public final static void setDesktop(JFrame gui, JDesktopPane dp){guiframe=gui; desktop=dp;}
public final static void setSuccess(boolean success){OK=success;}
public final static void setMessage(String message){msg=new String(message);}
// get all we have stored
public final static boolean isSuccess(){return OK;}
public final static String getMessage(){return msg;}
public final static int[] getParTableWidth(){return ParTableWidth;}
public final static String getDimDns(){return DimDns;}
public final static String getUserName(){return UserName;}
public final static String getGuiNode(){return GuiNode;}
public final static JDesktopPane getDesktop(){return desktop;}

public final static void setAccess(char[] access){
Access = new String(xCrypt.crypt("x1",new String(access)));
}
public final static String getAccess(){return Access;}
// Dimension adder for convenience
public final static Dimension addDimension(Dimension d1, Dimension d2){
return new Dimension((int)(d1.getWidth()+d2.getWidth()),(int)(d1.getHeight()+d2.getHeight()));
}
// To read images from jar file, we need this ugly stuff
public final static ImageIcon getIcon(String f){
    InputStream is = ClassLoader.getSystemResourceAsStream(f);
    try{
    return new ImageIcon(ImageIO.read(is));
    } catch(IOException e){}
    return null;
}

// Save geometries to xml file
public final static void Save(String file){
    str=new StringBuffer();
    str.append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    str.append("<Layout>\n");
    str.append("<WindowLayout>\n");
    // append window specs
    for(int i =0; i < layouts.size(); i++) str.append(layouts.elementAt(i).getXmlString()); 
    str.append("</WindowLayout>\n");
    // append table specs
    str.append("<TableLayout>\n");
    str.append(String.format("<Parameter width=\"%d",ParTableWidth[0]));
    for(int i=1;i<ParTableWidth.length;i++) str.append(","+Integer.toString(ParTableWidth[i]));
    str.append("\" />\n");
    str.append("</TableLayout>\n");
    str.append("</Layout>\n");
    try{
        FileWriter fw = new FileWriter(file);
        fw.write(str.toString());
        fw.close();
    }catch(IOException ioe){System.out.println("Error writing layout file");}
}

// used by restore
public final static void setWindowLayout(Element el){
String[] sx;
xLayout lo=createLayout(el.getTagName());
if(lo == null)lo=getLayout(el.getTagName());
sx=el.getAttribute("shape").toString().split(",");
lo.set(new Point(Integer.parseInt(sx[0]),Integer.parseInt(sx[1])),
    new Dimension(Integer.parseInt(sx[2]),Integer.parseInt(sx[3])),
    Integer.parseInt(el.getAttribute("columns").toString()),
    Boolean.parseBoolean(el.getAttribute("show").toString()));
}
// used by restore
public final static void setTableLayout(String name, Element e){
String[] sx;
Element el;
el=(Element)e.getElementsByTagName(name).item(0);
sx=el.getAttribute("width").toString().split(",");
for(int i=0;i<sx.length;i++)ParTableWidth[i]=Integer.parseInt(sx[i]);
}

// Restore geometry from xml file
public final static void Restore(String file){
String s;
int col;
boolean show;
Element root,elem,el;
try{
    DocumentBuilderFactory factory=DocumentBuilderFactory.newInstance();
    DocumentBuilder builder=factory.newDocumentBuilder();
    Document document=builder.parse(new File(file));
    root=document.getDocumentElement();
    elem=(Element)root.getElementsByTagName("WindowLayout").item(0); // only one WindowLayout!
    NodeList list=elem.getElementsByTagName("*"); // list of layout names
    for(int i=0;i<list.getLength();i++){
        el=(Element)list.item(i);
        setWindowLayout(el);
        // System.out.println("Layout elements "+el.getTagName());
    }
    elem=(Element)root.getElementsByTagName("TableLayout").item(0); // only one TableLayout!
    setTableLayout("Parameter",elem);
}catch(Exception e){}
}
}
