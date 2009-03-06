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
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.util.*;
import javax.swing.*;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import java.awt.geom.AffineTransform;
//***************************************************
/**
 * Panel with histogram display.
 * @author Hans G. Essel
 * @version 1.0
 * @see xPanelHisto
 */
public class xHisto extends JPanel 
                        implements xiPanelItem, MouseMotionListener, MouseListener, ActionListener, ComponentListener
//***************************************************
{
/** Can be used in setLogScale or redraw. */
public static final boolean LOG=true;
/** Can be used in setLogScale or redraw. */
public static final boolean LIN=false;
/** Line mode drawing. */
public static final int LINE=0;
/** Bar mode drawing. */
public static final int BAR=1;
/** recommended size x */
public final static int XSIZE=200;
/** recommended size y */
public final static int YSIZE=150;
/** recommended large size x */
public final static int XSIZE_LARGE=300;
/** recommended large size y */
public final static int YSIZE_LARGE=200;
private int ix;
private int iy;
private int iHead;
private int iFullHead;
private int iMin;
private int iMax;
private int iSum;
private int iMinl;
private int iMaxl;
private int iSuml;
private int iMaxc;
private int iMinc;
private int iXax;
private int iCur;
private int iposx;
private int iposy;
private int iInit;
private int iLastx;
private int iLasty;
private int iys;
private int iy0;
private int ixs;
private int ix0;
private int iID=0;
private int mode=0;
private boolean drag=true;
private boolean dragging=false;
private boolean isLog=false;
private boolean bar=false;
private String sHead;
private String sFullHead;
private String sCont;
private String sXaxis;
private String sMin;
private String sMax;
private String sCur;
private String sMaxc;
private String sMinc;
private String sSum;
private String sCoord;
private String sColor;
private String sLog;
private double fMin;
private double fMax;
private double fCur;
private double fVal;
private double fLast;
private double fScale;
private Color cCol;
private Color cColNorm;
private Color cColAlert;
private FontMetrics fm;
private Font tfont;
private BufferedImage iHisto;
private JMenuItem menit;
private Graphics2D gg;
private RenderingHints renderHint;
private ActionListener action;
private xHisto extHisto=null;
boolean boextHisto=false;
private JInternalFrame extFrame=null;
private int   iChan=0;
private float dData[];
private int iData[];
private int iPixelX[];
private int iPixelY[];
private double dDataY[];
private Color cColPanel = new Color(0.2f,0.2f,0.3f);
private Color cColBack  = new Color(0.0f,0.0f,0.0f);
private Color cColValue = new Color(1.0f,0.5f,0.5f);
private Color cColBord  = new Color(0.7f,1.0f,0.7f);
private Color cColText  = new Color(0.5f,1.0f,1.0f);
private Color cColTextV = new Color(0.5f,1.0f,0.5f);
//**************************************************************
/**
 * Create panel.
 * @param name Name of panel (returned by getName()).
 * @param head Headline.
 * @param cont Content lettering (Y-axis).
 * @param xaxis Content lettering (X-axix).
 * @param x Width.
 * @param y Height. 
 */
public xHisto(String name, String head, String cont, String xaxis, int x, int y)
//**************************************************************
{
    addMouseMotionListener(this);
    addMouseListener(this);
    addComponentListener(this);
    int l;
    ix=x;//400
    iy=y;//205
    iys = iy-40;
    iy0 = iy-20;
    ixs = ix-30;
    ix0 = 20;
    iInit=0;
    iLastx=-1;
    iLasty=-1;
    cColAlert = Color.red;
    sFullHead=name;
    setLettering(head, cont, xaxis);
    tfont = new Font("Dialog",Font.PLAIN,10);
    setFont(tfont);
    sMin = new String("Min=0");
    sMax = new String("Max");
    sMinc = new String("0");
    sMaxc = new String("0");
    sSum = new String("Int");
    sLog = new String("Lin");
    renderHint=new RenderingHints(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
    renderHint.put(RenderingHints.KEY_RENDERING,RenderingHints.VALUE_RENDER_QUALITY);
    // if(xSet.getDesktop()!=null){
    // JInternalFrame[] fr=xSet.getDesktop().getAllFrames();
    // for(int i=0;i<fr.length;i++){
        // if(fr[i].getTitle().equals(head)){
        // extFrame=(xInternalFrame)fr[i];
        // extHisto=(xHisto)(extFrame.getPanel());
        // boextHisto=true;
        // System.out.println("connect histogram "+head);
        // break;
    // }}}
}
/**
 * @param head Headline.
 * @param cont Content lettering (Y-axis).
 * @param xaxis Content lettering (X-axix).
 */
public void setLettering(String head, String cont, String xaxis){
    sHead=head;
    sCont = cont;
    sXaxis = xaxis;
    iLastx=-1;
}    
/**
 * If caller makes a clone of this Histogram (parent), and wants that clone to be
 * controlled by this Histogram (updated), it must set the clone to hasParent.
 * The clone is passed to its parent by setExternHisto together with a reference to
 * a JInternalFrame where the clone is displayed. This frame must be created by caller.
 * If frame is closed, the parent removes its child.
 */
protected void hasParent(){
boextHisto=true; // true: cannot create child
}
/**
 * Attach a second histogram to this. Second histogram will be updated from this.
 * @param exthisto Typically a clone with large size to be displayed in an extra frame.
 * @param extframe Extra frame for the clone. Must be handled by caller.
 */
protected void setExternHisto(xHisto exthisto, JInternalFrame extframe){
extHisto=exthisto;
extFrame=extframe;
boextHisto=true;
}
// If frame is detected to be gone, call this function to clean up.
private void removeHisto(){
extFrame=null;
extHisto=null;
boextHisto=false;
}
// xiPanelItem
public void setActionListener(ActionListener actionlistener){action=actionlistener;}
public JPanel getPanel() {return this;}
public String getName(){return sFullHead;}
public void setID(int i){iID=i;}
public int getID(){return iID;}
public Point getPosition(){return new Point(getX(),getY());};
public Dimension getDimension(){return new Dimension(ix,iy);};
public void setSizeXY(){setPreferredSize(new Dimension(ix,iy));}
public void setSizeXY(Dimension dd){
    ix=(int)dd.getWidth();
    iy=(int)dd.getHeight();
    iys = iy-40;
    iy0 = iy-20;
    ixs = ix-30;
    ix0 = 20;
    iInit=0;
    iLastx=-1;
    iLasty=-1;
    setPreferredSize(dd);
}
public int getSizeX(){return ix;}
public int getSizeY(){return iy;}
public void setBar(boolean drawbar){
bar=drawbar;
if(bar)mode=BAR;
else mode=LINE;
}
public int getMode(){return mode;}
public void setMode(int Mode){
mode=Mode;
if(mode==BAR)bar=true;
else bar=false;
}

public void setColorBack(Color color){cColBack=color;}
/**
 * Set color by name.
 * @param colorname (Red, Green, Blue, Yellow, Cyan, Magenta).
 */
public void setColor(String colorname){
    if(colorname.equals("Red"))    {sColor=colorname; cCol=new Color(1.0f,0.0f,0.0f);}
    if(colorname.equals("White"))  {sColor=colorname; cCol=new Color(1.0f,1.0f,1.0f);}
    if(colorname.equals("Blue"))   {sColor=colorname; cCol=new Color(0.3f,0.5f,1.0f);}
    if(colorname.equals("Green"))  {sColor=colorname; cCol=new Color(0.0f,1.0f,0.0f);}
    if(colorname.equals("Yellow")) {sColor=colorname; cCol=new Color(1.0f,1.0f,0.0f);}
    if(colorname.equals("Cyan"))   {sColor=colorname; cCol=new Color(0.5f,1.0f,1.0f);}
    if(colorname.equals("Magenta")){sColor=colorname; cCol=new Color(1.0f,0.5f,1.0f);}
}
public void setColor(Color c){
    cCol=c;
    cColNorm = c;
}
public String getColor(){return sColor;}
public void setLogScale(boolean log){isLog=log;if(log)sLog=new String("Log");else sLog=new String("Lin");}
public boolean getLogScale(){return isLog;}
// only used by external panels to resize.
private void askSize(){
int x,y;
    String str=JOptionPane.showInputDialog("Enter size: x,y",
        new String(Integer.toString(ix)+","+Integer.toString(iy)));
    String f[]=str.split(",");
    x=Integer.parseInt(f[0]);
    y=Integer.parseInt(f[1]);
    setSizeXY(new Dimension(x,y));
}
/**
 * Create a new histogram from existing one. When clone shall be attached
 * to parent histogram, typically these settings must be applied:<br> 
 * clone.setSizeXY(new Dimension(XSIZE_LARGE,YSIZE_LARGE));<br>
 * clone.hasParent();<br>
 * parent.setExternHisto(clone,Frame);
 * @param name Name of clone.
 */
public xHisto getClone(String name){
    extHisto=new xHisto(sFullHead, sHead, sCont, sXaxis, ix, iy); 
    extHisto.setBar(bar);
    extHisto.setColor(cCol);
    extHisto.setColorBack(cColBack);
    extHisto.setLogScale(isLog);
    return extHisto;
}
//***************************************************
/**
 * Called by repaint, calls update.
 */
public void paintComponent(Graphics g)  // called by update
//***************************************************
{
    update(g); // update is not called by repaint!
    Graphics2D g2d = (Graphics2D)g;
    g2d.drawImage(iHisto,new AffineTransform(1f,0f,0f,1f,0,0),this);
}

//***************************************************
/**
 * Overwriting update method we avoid clearing the
 * graphics. This would cause flickering.
 * Update is not called by repaint!
 */
public void update(Graphics g)  
//***************************************************
{
// System.out.print("Update, init="+iInit+" draw="+iLastx);
// static graphics into memory image
    if(iInit == 0) {
        iHisto = new BufferedImage(ix,iy,BufferedImage.TYPE_INT_RGB);
        gg = iHisto.createGraphics();
        gg.addRenderingHints(renderHint);
        fm = gg.getFontMetrics();
        iInit = 1;
    }
// redraw only if necessary
    if(iLastx == -1) {
        fm = gg.getFontMetrics();
        iMinl = fm.stringWidth(sMin);
        iMaxl = iy - 3 - 4 - fm.stringWidth(sMax);
        iSuml = ix - 3 - fm.stringWidth(sSum);
        iFullHead = fm.stringWidth(sFullHead);
        iHead = (ix - fm.stringWidth(sSum) - fm.stringWidth(sHead)) / 2;
        iMinc = fm.stringWidth(sMinc);
        iMaxc = ix - 3 - fm.stringWidth(sMaxc);
        iXax  = (ix - fm.stringWidth(sXaxis)) / 2;
        gg.setColor(cColPanel);
        gg.fillRect(0,0,ix,iy); // 18
        gg.setColor(cColText);
        //gg.drawRect(0,0,ix-1,iy);
        //gg.drawRect(1,1,ix-3,iy-2);
        gg.setColor(cColBack);
        gg.drawRect(0,0,ix,iy);
        gg.fillRect(20,18,ix-30,iy-38);
        gg.setColor(cColText);
        gg.drawRect(20,18,ix-30,iy-38);
        gg.drawString(sSum,iSuml,14);
        gg.drawString(sMinc,20,iy-4);
        gg.drawString(sMaxc,iMaxc,iy-4);
        gg.setColor(cColTextV);
        if((iFullHead-iSuml+ix+20)>ix) gg.drawString(sHead,20,14);
        else                     gg.drawString(sFullHead,20,14);
        gg.drawString(sXaxis,iXax,iy-4);
        gg.drawString(sLog,ix-30,30);
        gg.setColor(cCol);
        if(iPixelX != null){
        if(bar){
        	int ixw=iPixelX[1]-iPixelX[0];
            for(int i=0;i<iChan;i++)gg.drawRect(iPixelX[i],iPixelY[i],ixw,iy0-iPixelY[i]);
            }else if(iChan>0)gg.drawPolyline(iPixelX,iPixelY,iChan);
        }
// save current transform
        AffineTransform af = gg.getTransform();
        gg.rotate(-Math.PI/2.0); // rotate left 90
        gg.translate(-iy-4,0);
// whole image is rotated around upper left corner
// Therefore we must pull down y
        gg.setColor(cColText);
        gg.drawString(sMax,iMaxl,14);
        gg.setColor(cColTextV);
        gg.drawString(sCont,24,14);
        // restore transform
        gg.setTransform(af);
        // dont draw again
        gg.setColor(cCol);
        iLastx=-2;
    }
// draw static image
//paint(g);
}

//****************************************************
/**
 * Redraw with new value.
 * @param channels Number of channels
 * @param iBuffer Integer field with data.
 * @param draw True: call redraw, otherwise no repaint.
 */
public void redraw(int channels, int[] iBuffer, boolean draw){
	if(dragging) return; // mouse is dragging, do not disturb
    iChan=channels;
    iData = iBuffer;
    if(iPixelY == null){
        iPixelY = new int[iChan];
        iPixelX = new int[iChan];
        dDataY = new double[iChan];
    } else
        if(iPixelY.length != iChan){
        iPixelY = new int[iChan];
        iPixelX = new int[iChan];
        dDataY = new double[iChan];
        }
    if(draw)redraw();
    if(extFrame != null){
        if(extFrame.isClosed()) removeHisto();
        }
    if(extHisto != null) extHisto.redraw(channels, iBuffer, draw);
}
//****************************************************
/**
 * Final redraw. Calls repaint.
 */
public void redraw()
//***************************************************
// integer
{
    int l,iMaxold;
    double dlog;
    double dMax;
    double off0;

	if(dragging) return; // mouse is dragging, do not disturb
    iLastx=-1;

    iMaxold=iMax;
    iMax=0;
    iSum=0;
    if(isLog) {
        dMax=0.0;
        dlog = 1.0/Math.log(10.);
        off0 = dlog*Math.log(0.1);
        for(l=0;l<iChan;l++) {
            iSum += iData[l];
            if(iData[l] > iMax) iMax=iData[l];
            if(iData[l] <= 0) dDataY[l] = off0;
            else                dDataY[l] = dlog * Math.log((double)iData[l]);
            if(dDataY[l] > dMax) dMax=dDataY[l];
        }
        iMax=(int)((float)iMax*1.5);
        dMax=1.2*dMax-off0;
        for(l=0;l<iChan;l++) {
            iPixelY[l]= iy0 - (int) ((dDataY[l]-off0) * (double)iys / dMax);
            iPixelX[l]= ix0 + (int) ((float)l * (float)ixs / (float)iChan);
        }
    } else {
        for(l=0;l<iChan;l++) {
            iSum += iData[l];
            if(iData[l] > iMax) iMax=iData[l];
        }
        iMax=(int)((float)iMax*1.5);
        for(l=0;l<iChan;l++) {
            iPixelY[l]= iy0 - (int) ((float)iData[l] * (float)iys / (float)iMax);
            iPixelX[l]= ix0 + (int) ((float)l * (float)ixs / (float)iChan);
        }
    }
    sMin = new String("Min=0");
    sMax = new String(Integer.toString(iMax));
    sMinc = new String("0");
    sMaxc = new String(Integer.toString((iChan-1)));
    sSum = new String("Int="+iSum);
    repaint();
}
//****************************************************
/**
 * Final redraw. Calls repaint.
 * @param head Headline.
 * @param cont Content lettering (Y-axis).
 * @param xaxis Content lettering (X-axix).
 * @param dBuffer Data field.
 * @param channels Number of channels
 * @param log Logarithmic scale?
 * @param c Color.
 */
public void redraw(String head, String cont, String xaxis,
        float[] dBuffer, int channels, boolean log, Color c)
//***************************************************
// float
{
    int l;
    double dlog;
    double dMax;
    double off0;

	if(dragging) return; // mouse is dragging, do not disturb
    iChan=channels;
    sHead=head;
    sCont = cont;
    sXaxis = xaxis;
    cCol = c;
    cColNorm = c;
    //  dData = dBuffer;
    iPixelY = new int[iChan];
    iPixelX = new int[iChan];
    iLastx=-1;

    iMax=0;
    iSum=0;
    if(isLog) {
        dMax=0.0;
        dlog = 1.0/Math.log(10.);
        off0 = dlog*Math.log(0.1);
        dDataY = new double[iChan];
        for(l=0;l<iChan;l++) {
            iSum += dBuffer[l];
            if(dBuffer[l] > iMax) iMax=(int)dBuffer[l];
            if(dBuffer[l] <= 0) dDataY[l] = off0;
            else                dDataY[l] = dlog * Math.log((double)dBuffer[l]);
            if(dDataY[l] > dMax) dMax=dDataY[l];
        }
        dMax=dMax-off0;
        for(l=0;l<iChan;l++) {
            iPixelY[l]= iy0 - (int) ((dDataY[l]-off0) * (double)iys / dMax);
            iPixelX[l]= ix0 + (int) ((float)l * (float)ixs / (float)iChan);
        }
    } else {
        for(l=0;l<iChan;l++) {
            iSum += dBuffer[l];
            if(dBuffer[l] > iMax) iMax=(int)dBuffer[l];
        }
        for(l=0;l<iChan;l++) {
            iPixelY[l]= iy0 - (int) ((float)dBuffer[l] * (float)iys / (float)iMax);
            iPixelX[l]= ix0 + (int) ((float)l * (float)ixs / (float)iChan);
        }
    }
    sMin = new String("Min=0");
    sMax = new String("Max="+iMax);
    sMinc = new String("0");
    sMaxc = new String(Integer.toString((iChan-1)));
    sSum = new String("Int="+iSum);
    repaint();
}
// Context menu execution (ActionListener)
public void actionPerformed(ActionEvent a){
    String c=a.getActionCommand();
    setColor(c);
    // Only for external histogram. Parent frame must resize (see xInternalFrame).
    // Therefore the frame is here the additional action listener.
    if(c.equals("Size")){
        askSize();
        if(action!=null) action.actionPerformed(new ActionEvent(this,getID(),"MyResize"));
    }
    // External histogram is created and displayed in an extra frame by
    // xPanelHisto, see "Large". Normal histograms have xPanelHisto as
    // additional action listener.
    else if(c.equals("Large")){
        if((action!=null)&(!boextHisto)) {
        boextHisto=true;
        action.actionPerformed(new ActionEvent(this,getID(),"Large"));
        }}
    else if(c.equals("setLin")){setLogScale(false);redraw();}
    else if(c.equals("setLog")){setLogScale(true);redraw();}
    else if(c.equals("Barchart")){setBar(true);redraw();}
    else if(c.equals("Histogram")){setBar(false);redraw();}
    drag=true; // enable cross cursor drawing
}
/**
 * Context menu definition (RMB)
 */
 public void mousePressed(MouseEvent me) {
    if(me.getButton()==MouseEvent.BUTTON3){
        drag=false; // prevent cross cursor drawing
        JPopupMenu men=new JPopupMenu();
        menit=new JMenuItem(sFullHead);
        men.add(menit);
        if(bar)	menit=new JMenuItem("Histogram");
        else 	menit=new JMenuItem("Barchart");
        men.add(menit);
        menit.addActionListener(this);
        if(isLog)	menit=new JMenuItem("setLin");
        else	menit=new JMenuItem("setLog");
        men.add(menit);
        menit.addActionListener(this);
        men.addSeparator();
        JMenu colmen=new JMenu("Colors");
        
        menit=new JMenuItem("White");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Red");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Green");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Blue");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Yellow");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Magenta");
        colmen.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Cyan");
        colmen.add(menit);
        menit.addActionListener(this);
        men.add(colmen);
        men.addSeparator();
        if(!boextHisto){
            menit=new JMenuItem("Large");
            men.add(menit);
            menit.addActionListener(this);
        } else { // we are external
        if(extHisto==null){
            menit=new JMenuItem("Size");
            men.add(menit);
            menit.addActionListener(this);
        }}
        // men.addSeparator();
        // menit=new JMenuItem("Discard");
        // men.add(menit);
        // menit.addActionListener(this);
//men.setLightWeightPopupEnabled(true);
        men.setVisible(true);
        men.show(me.getComponent(),me.getX(),me.getY());
    }
    else if(me.getButton()==MouseEvent.BUTTON1)dragging=true;
}
//****************************************************
public void mouseClicked(MouseEvent me)
//***************************************************
{
// called after released if short click
// System.out.println("Click Repaint ");
// if(iInit != 0){
    // gg.setXORMode(Color.yellow);
    // if(iLastx >= 0)
    // {
    // gg.drawString(sCoord,iposx,iposy);
    // gg.drawLine(20,iLasty,ix-10,iLasty); // horizontal
    // gg.drawLine(iLastx,18,iLastx,iy-20); // vertical
    // iLastx = -1;
    // }
    // gg.setPaintMode();
    // }
    // repaint();
}
//****************************************************
public void mouseEntered(MouseEvent me)
//***************************************************
{
}
//****************************************************
public void mouseExited(MouseEvent me)
//***************************************************
{
}
//****************************************************
public void mouseReleased(MouseEvent me)
//***************************************************
{
// this is called before click
    drag=true; // enable cross cursor drawing
    if(me.getButton()==MouseEvent.BUTTON1){
    	dragging=false;
    	iLastx=-1;
        repaint();
    }}
//****************************************************
public void mouseMoved(MouseEvent me)
//***************************************************
{
}
//****************************************************
public void mouseDragged(MouseEvent me)
//***************************************************
{
// no click event, but release
    if(drag){
        int x =  me.getX();
        int y =  me.getY();
        if((x>20) && (x<(ix-10)) && (y>18) && (y<(iy-20))){
            gg.setXORMode(Color.yellow);
            if(iLastx >= 0) {
                gg.drawString(sCoord,iposx,iposy);
                gg.drawLine(20,iLasty,ix-10,iLasty); // horizontal
                gg.drawLine(iLastx,18,iLastx,iy-20); // vertical
            }
            iLastx =  x;
            iLasty =  y;
            int i = (iLastx-ix0)*iChan/ixs;
            int j = (-iLasty+iy0)*(iMax/iys);
            if(i < 0) i = 0;  if(i >= iChan) i = iChan-1;
            if(j > 100000) sCoord = String.format(" %d, %10.5e",i,(float)j);
            else           sCoord = String.format(" %d, %d",i,j);
            iposx=iLastx+3;
            iposy=iLasty-4;
            if(iLastx > ix/2) iposx=iLastx-3-fm.stringWidth(sCoord);
            if(iLasty < 40)   iposy=iLasty+26;
            gg.drawString(sCoord,iposx,iposy);
            gg.drawLine(20,iLasty,ix-10,iLasty);
            gg.drawLine(iLastx,18,iLastx,iy-20);
            gg.setPaintMode();
            repaint();
        }
    }}
// Component listener
public void componentHidden(ComponentEvent e) {}
public void componentMoved(ComponentEvent e) {}
public void componentShown(ComponentEvent e) {}
public void componentResized(ComponentEvent e) {
setSizeXY(getSize());
}
}

