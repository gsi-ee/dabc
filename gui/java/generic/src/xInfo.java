package xgui;
import java.awt.*;
import java.awt.image.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import java.awt.geom.AffineTransform;

public class xInfo extends JPanel implements xiPanelGraphics, MouseListener, ActionListener
{
public final static int XSIZE=600;
public final static int YSIZE=14;
private  int ix;
private  int iy;
private int iSeverity=0;
private int iDim=0;
private int iYpos;
private int iInit;
private String sHead;
private String sSev[]={"S","W","E","F","L"};
private String sInfo;
private Color cColBack  = new Color(0.0f,0.0f,0.0f);
private Color cColText  = new Color(1.0f,1.0f,1.0f);
private Color cColValue = new Color(1.0f,0.5f,0.5f);
private Color cColNosp  = new Color(0.5f,0.5f,0.5f);
private FontMetrics fm;
private Font tfont;
private BufferedImage imStore;
private Graphics2D gg;
private int i;
private int ii;
private JMenuItem menit;

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
 * Creates a Info canvas.
 * @param head name of parameter displayed
 * @param xlength size of canvas in pixels
 * @param ylength size of canvas in pixels
 */
public xInfo(String head, int xlength, int ylength){
    initInfo(head,xlength,ylength);
    addMouseListener(this);
}
/**
 * Initializes a Info canvas (called by constructor).
 * @param head name of parameter displayed
 * @param xlen size of canvas in pixels
 * @param ylen size of canvas in pixels
 */
public void initInfo(String head, int xlen, int ylen){
    if(xlen>0) ix=xlen;
    if(ylen>0) iy=ylen;
    if(ix<400)ix=400;
    if(iy<14)iy=14;
    setSize(ix,iy);
    sHead = new String(head);
    sInfo = new String("%NOVALUE%");
    tfont = new Font("Dialog",Font.PLAIN,10);
    setFont(tfont);
    cColValue=cColNosp;
}
public void setColorBack(Color color){cColBack=color;}
/**
 * Set color.
 * @param colorname (Red, Green, Blue, Yellow, Cyan, Magenta).
 */
public void setColor(String colorname){
    if(colorname.equals("Red"))    cColValue=new Color(1.0f,0.0f,0.0f);
    if(colorname.equals("Blue"))   cColValue=new Color(0.5f,0.5f,1.0f);
    if(colorname.equals("Green"))  cColValue=new Color(0.0f,1.0f,0.0f);
    if(colorname.equals("Yellow")) cColValue=new Color(1.0f,1.0f,0.0f);
    if(colorname.equals("Cyan"))   cColValue=new Color(0.5f,1.0f,1.0f);
    if(colorname.equals("Magenta"))cColValue=new Color(1.0f,0.5f,1.0f);
    if(colorname.equals("White"))  cColValue=new Color(1.0f,1.0f,1.0f);
    if(colorname.equals("Gray"))   cColValue=new Color(0.5f,0.5f,0.5f);
}
public void setID(int i){}
/**
 * Set preferred size to the one specified in constructor. Called from the panels.
 */
public void setSizeXY(){
    setPreferredSize(new Dimension(ix,iy));
}
/**
 * Set preferred size to the one specified in constructor. Called from the panels.
 */
public void setSizeXY(Dimension dd){
    setPreferredSize(dd);
}
public String getName(){return sHead;}

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
// Overwriting update method we avoid clearing the
// graphics. This would cause flickering
// update is not called by repaint!

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
    gg.setColor(cColText);
    gg.fillRect(2,2,iy-4,iy-4);
    gg.setColor(cColNosp);
    gg.fillRect(3,3,iy-5,iy-5);
    gg.setColor(cColValue);
    gg.fillRect(3,3,iy-6,iy-6);
    gg.drawString(sInfo,iy+2,iy-2);
}
/**
 * Redraw without changes.
 */
public void redraw(){repaint();}
/**
 * Redraw with new value
 * @param severity (0: Success, 1: Warning, 2: Error, 3: Fatal)
 * @param colorname (Red, Green, Blue, Yellow, Cyan, Magenta).
 * @param value Short string describing info
 */
public void redraw(int severity, String colorname, String value, boolean draw){
if(severity == 0) sInfo=new String(value);
else if(severity == 1) sInfo=new String(sHead+value);
else sInfo=new String(sHead+value);
    setColor(colorname);
    if(draw)redraw();
}
}
