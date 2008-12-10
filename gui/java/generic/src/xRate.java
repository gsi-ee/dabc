package xgui;
import java.awt.*;
import java.awt.image.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import java.awt.geom.AffineTransform;

/**
 * Graphic rate meter (bar). Can be used as item in xPanelGraphics.
 * @author Hans G. Essel
 * @version 1.0
 * @see xPanelGraphics
 */
public class xRate extends JPanel implements xiPanelItem, MouseListener, ActionListener
{
/** recommended size x */
public final static int XSIZE=400;
/** recommended size y */
public final static int YSIZE=14;
private int ix;
private int iy;
private int iSeverity=0;
private int iDim=0;
private int iYpos;
private int iInit;
private int iID;
private int barsize,barwidth,barposy,barposx;
double Min,Max,Fact,Value,ValMax;
private String sHead;
private String sName;
private String sUnits;
private String sValue;
private String sMin;
private String sMax;
private int iMaxL, iValL;
private boolean withHeader;
private boolean auto=false;
float colR, colG, colB;
private Color cColBack  = new Color(0.0f,0.0f,0.0f);
private Color cColText  = new Color(1.0f,1.0f,1.0f);
private Color cColPanel = new Color(0.5f,1.0f,0.5f);
private Color cColValue;
private Color cColMax;
private Color cColBar;
private Color cColCur=null;
private Color cColNosp  = new Color(0.2f,0.2f,0.2f);
private FontMetrics fm;
private Font tfont;
private BufferedImage imStore;
private Graphics2D gg;
private int i,ii;
private JMenuItem menit;
private ActionListener action;

// Context menu execution (ActionListener)
public void actionPerformed(ActionEvent a){
    String c=a.getActionCommand();
    if(c.equals(sHead));
}
// context menu definition (MouseListener)
public void mousePressed(MouseEvent me) {
    if(me.getButton()==MouseEvent.BUTTON3){
        JPopupMenu men=new JPopupMenu();
        menit=new JMenuItem(sHead);
        men.add(menit);
        menit.addActionListener(this);
        men.setVisible(true);
        men.show(me.getComponent(),me.getX(),me.getY());
    }
}
public void mouseClicked(MouseEvent me){}
public void mouseEntered(MouseEvent me){}
public void mouseExited(MouseEvent me){}
public void mouseReleased(MouseEvent me){}

/**
 * Creates a Rate canvas.
 * @param head Name of parameter displayed.
 * @param xlength Size of canvas in pixels.
 * @param ylength Size of canvas in pixels.
 */
public xRate(String head, String name, boolean header, int xlength, int ylength, double min, double max, String color){
    withHeader=header;
    initRate(head,name,xlength,ylength, min, max, color);
    addMouseListener(this);
}
/**
 * Initializes a Rate canvas (called by constructor).
 * @param head Name of parameter displayed.
 * @param xlen Size of canvas in pixels.
 * @param ylen Size of canvas in pixels.
 */
public void initRate(String head, String name, int xlen, int ylen, double min, double max, String color){
    setColor(color);
    if(xlen>0) ix=xlen;
    if(ylen>0) iy=ylen;
    if(ix<400)ix=400;
    if(iy<14)iy=14;
    if(withHeader)iy=28;
    barposx=200; // x of bar
    barsize=150; // x length of bar
    barwidth=14; // width of bar
    Min=min;
    Max=max;
    if(Min == Max) {auto=true;Max=Min+1.0;}
    Fact = (double) barsize / (Max - Min);
    setSize(ix,iy);
    sHead = new String(head);
    sName = new String(name);
    sValue = new String("none");
    sMax=new String(String.format("%6.0f",Max));
    tfont = new Font("Dialog",Font.PLAIN,10);
    setFont(tfont);
}
private void setColor(float red, float green, float blue){
    cColValue = new Color(red,green,blue);
    red   = red   - 0.2f;
    green = green - 0.2f;
    blue  = blue  -0.2f;
    if(red < 0) red=0;
    if(green < 0) green=0;
    if(blue < 0) blue=0;
    cColMax   = new Color(red,green,blue);
    red   = red   - 0.2f;
    green = green - 0.2f;
    blue  = blue  -0.2f;
    if(red < 0) red=0;
    if(green < 0) green=0;
    if(blue < 0) blue=0;
    cColBar   = new Color(red,green,blue);
}
public void setColorBack(Color color){cColBack=color;}
/**
 * Set color.
 * @param colorname (Red, Green, Blue, Yellow, Cyan, Magenta).
 */
public void setColor(String colorname){
    if(colorname.equals("Red"))    setColor(1.0f,0.0f,0.0f);
    if(colorname.equals("Blue"))   setColor(0.5f,0.5f,1.0f);
    if(colorname.equals("Green"))  setColor(0.0f,1.0f,0.0f);
    if(colorname.equals("Yellow")) setColor(1.0f,1.0f,0.0f);
    if(colorname.equals("Cyan"))   setColor(0.5f,1.0f,1.0f);
    if(colorname.equals("Magenta"))setColor(1.0f,0.5f,1.0f);
    if(colorname.equals("White"))  setColor(1.0f,1.0f,1.0f);
    if(colorname.equals("Gray"))   setColor(0.5f,0.5f,0.5f);
}
public double getMin(){return Min;}
public double getMax(){return Max;}
// xiPanelGraphics
public void setActionListener(ActionListener actionlistener){action=actionlistener;}
public JPanel getPanel() {return this;}
public String getName(){return sHead;}
public void setID(int i){iID=i;}
public int getID(){return iID;}
public Point getPosition(){return new Point(getX(),getY());};
public Dimension getDimension(){return new Dimension(ix,iy);};
public void setSizeXY(){setPreferredSize(new Dimension(ix,iy));}
public void setSizeXY(Dimension dd){setPreferredSize(dd);}

public String getHeader(){return sHead;}

/**
 * Called by repaint, calls update.
 */
public void paintComponent(Graphics g){
// draw image
// System.out.println("paint");
    if(imStore == null){
        imStore = new BufferedImage(ix,iy,BufferedImage.TYPE_INT_RGB);
        gg = imStore.createGraphics();
    }
    update(g); // update is not called by repaint!
    Graphics2D g2d = (Graphics2D)g;
    g2d.drawImage(imStore,new AffineTransform(1f,0f,0f,1f,0,0),this);
}
/**
 * Overwriting update method we avoid clearing the
 * graphics. This would cause flickering
 * update is not called by repaint!
 */
public void update(Graphics g){
// static graphics into memory image
    int ixtemp;
// System.out.println("update init "+iInit);
    if(iInit == 0){
        if(imStore == null){
            imStore = new BufferedImage(ix,iy,BufferedImage.TYPE_INT_RGB);
            gg = imStore.createGraphics();
        }
        fm = gg.getFontMetrics();
        iInit = 1;
    } // init
    gg.setColor(cColBack);
    gg.fillRect(0,0,ix,iy);
    if(withHeader){ // iy is then 28
        barposy=iy/2;
        gg.setColor(cColBack); // instead of Nosp
        gg.fillRect(0,0,ix,iy/2);
        gg.setColor(cColPanel);
        gg.drawString(sHead,2,iy/2-2);
    }
    else barposy=0;
    iMaxL  = fm.stringWidth(sMax);
    iValL  = fm.stringWidth(sValue);
    gg.setColor(cColText);
    gg.drawString(sName,2,iy-2);
    gg.drawString(sMax,ix-2-iMaxL,iy-2);
    gg.setColor(cColBar);
    gg.fillRect(barposx,barposy,barsize,barwidth);
    gg.setColor(cColMax);
    gg.fillRect(barposx,barposy,(int)(ValMax*Fact),barwidth);
    gg.setColor(cColText);
    gg.fillRect(barposx,barposy,(int)(Value*Fact),barwidth);
    gg.setColor(cColNosp);
    gg.fillRect(barposx+2,barposy+2,(int)(Value*Fact)-2,barwidth-2);
    gg.setColor(cColValue);
    gg.fillRect(barposx+2,barposy+2,(int)(Value*Fact)-4,barwidth-4);
    gg.drawString(sValue,barposx-2-iValL,iy-2);
    if(cColCur != null){
    cColValue=cColCur;
    cColCur=null;
    }
}
/**
 * Redraw without changes (repaint).
 */
public void redraw(){repaint();}
/**
 * Redraw with new value.
 * @param value New value.
 * @param draw True for redraw.
 */
public void redraw(double value, boolean draw){
    if(auto && (value > Max)){
        int dec = (int)Math.log10(2.0*value);
        double fMax=Math.pow(10.,(double)dec);
        Max=fMax;
        while(value>fMax)fMax+=Max;
        Max=2.0*fMax;
        Fact = (double) barsize / (Max - Min);
    }
    Value=value;
    if(Value > Max) {
        cColCur=cColValue;
        setColor("Red");
        Value=Max;
    }
    if(Value < Min) Value=Min;
    if(Value > ValMax)ValMax=Value;
    sValue=new String(String.format("%6.1f",value));
    sMax=new String(String.format("%6.0f",Max));
    if(draw)redraw();
}
}
