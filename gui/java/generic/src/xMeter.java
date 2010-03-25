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
import java.awt.image.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import java.awt.geom.AffineTransform;
import javax.swing.JInternalFrame;
import java.lang.Double;
/**
 * Graphic item rate meter. Can be displayed in two fixed sizes.
 * @author Hans G. Essel
 * @version 1.0
 * @see xPanelMeter
 */
public class xMeter extends JPanel implements xiPanelItem, MouseListener, ActionListener {
private  int ix;
private  int iy;
/** Arc mode = 0, half circle with indicator. */
public final static int ARC=0; 
/** Bar mode = 1, horizontal. */
public final static int BAR=1; 
/** Trend histogram = 2. */
public final static int TREND=2;
/** Statistics histogram = 3. */
public final static int STAT=3;
/** normal size x */
public final static int XSIZE=100;
/** normal size y */
public final static int YSIZE=90;
/** large size x */
public final static int XSIZE_LARGE=200;
/** large size y */
public final static int YSIZE_LARGE=102;
private int iAngle;
private int iAngleLast;
private int iHead;
private int iMinL;
private int iMaxL;
private int iUnitL;
private int iMin;
private int iMax;
private int iCurL;
private int iyOff;
private int iDim=0;
private int iYpos, iXpos=0;
private int iInit;
private int iMode, defMode;
private int ixpol[]={1,2,3};
private int iypol[]={1,2,3};
private int ixtick[]={1,2,3,4,5};
private int iytick[]={1,2,3,4,5};
private boolean boArc=true;
private boolean boBar=false;
private boolean boTrend=false;
private boolean boStat=false;
private boolean isLog=false, defLog=false;
private boolean boAutoScale=true, defAutoScale=true;
private boolean boLarge=false;
private String sFullHead1;
private String sFullHead2;
private String sFullHead3;
private String sHead;
private String sMin;
private String sMax;
private String sCont;
private String sSum;
private String sCur;
private String sUnits;
private String sColor;
private double fMin,defMin;
private double fMax,defMax;
private double fAngle;
private double fCur;
private double fVal;
private double fLast;
private double fTemp;
private double fScale;
private double fScaleBar;
private double fScaleTrend; //    red    green    blue
private Color cColBack  = new Color(0.0f,0.0f,0.0f);
private Color cColHead  = new Color(0.8f,1.0f,0.8f);
private Color cColText  = new Color(1.0f,1.0f,1.0f);
private Color cColValue = new Color(1.0f,0.5f,0.5f);
private Color cColNorm  = Color.darkGray;
private Color cColAlert = new Color(1.0f,0.5f,0.0f);
private Color cColNosp  = new Color(0.6f,0.6f,0.6f);
private Color cColCur;
// private Color cColBack  = new Color(0.0f,0.5f,0.5f);
// private Color cColHead  = new Color(0.0f,1.0f,0.5f);
// private Color cColText  = new Color(0.5f,1.0f,1.0f);
// private Color cColPanel = new Color(0.0f,0.0f,0.5f);
// private Color cColValue = new Color(1.0f,0.5f,0.5f);
// private Color cColNorm  = new Color(1.0f,1.0f,1.0f);
// private Color cColAlert = new Color(1.0f,0.0f,0.0f);
private FontMetrics fm;
private Font tfont;
private BufferedImage imStore;
private Graphics2D gg;
private double fTrend[];
private double fStat[];
private int   ixVal[];
private int   iyVal[];
private int   iyVall[];
private int iNum;
private int iID=0;
private int iTimes;
private int i;
private int ii;
private double ddSum;
private double ddCont;
private boolean valueValid=false;
private JMenuItem menit;
private RenderingHints renderHint;
private ActionListener action;
private xMeter extMeter;
boolean boextMeter=false;
private JInternalFrame extFrame=null;

/**
 * Creates a meter canvas.
 * @param mode BAR, ARC, TREND, STAT
 * @param name Name of parameter displayed
 * @param min Parameter value range
 * @param max Parameter value range
 * @param xlength Size of canvas in pixels (XSIZE)
 * @param ylength Size of canvas in pixels (YSIZE)
 * @param c Color of markers
 */
public xMeter(int mode, String name, double min, double max, int xlength, int ylength, Color c){
 iMode=mode;
 if(mode==BAR)  boBar=true;
    else if(mode==TREND)boTrend=true;
    else if(mode==STAT) boStat=true;
    else        boArc=true;
    sFullHead1=new String(name);
    sFullHead2=new String("%none%");
    sFullHead3=new String("%none%");
    sUnits=new String("Rate");
    renderHint=new RenderingHints(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
    renderHint.put(RenderingHints.KEY_RENDERING,RenderingHints.VALUE_RENDER_QUALITY);
    initMeter(name,min,max,xlength,ylength,c);
    addMouseListener(this);
    // look if external frame is still there
    // if(xSet.getDesktop()!=null){
    // JInternalFrame[] fr=xSet.getDesktop().getAllFrames();
    // for(int i=0;i<fr.length;i++){
        // if(fr[i].getTitle().equals(head)){
        // extFrame=(xInternalFrame)fr[i];
        // extMeter=(xMeter)(extFrame.getPanel());
        // boextMeter=true;
        // System.out.println("connect meter "+head);
        // break;
    // }}}
}
/**
 * Initializes a meter canvas (called by constructor).
 * @param name Name of parameter displayed
 * @param min Parameter value range
 * @param max Parameter value range
 * @param xlen Size of canvas in pixels
 * @param ylen Size of canvas in pixels
 * @param c Color of markers
 */
private void initMeter(String name, double min, double max, int xlen, int ylen, Color c){
    if(xlen>0) ix=xlen;
    if(ylen>0) iy=ylen;
    if(ix<XSIZE)ix=XSIZE;
    if(iy<YSIZE)iy=YSIZE;
    iAngle = 0;
    iYpos=iy-49;
    iXpos=14;
    iDim=ix-5-iXpos;
    fCur=min;
    // if(boArc){
        // ix=XSIZE;
        // iy=YSIZE;
    // }
    // if(boBar){
        // iy=YSIZE;
    // }
    if(boTrend||boStat){
        iYpos = iy-58; // was 17
        fCur=0.0;
        fTrend   = new double[iDim];
        fStat   = new double[iDim];
        ixVal = new int[iDim];
        iyVal = new int[iDim];
        // iyOff=iYpos+iy-35; // was 17+YSIZE-35=57, is YSIZE-18
        iyOff=iy-18; // was 17+YSIZE-35=57, is YSIZE-18
        for(int l=0;l<iDim;l++){
            fStat[l]=0.0;
            fTrend[l]=0.0;
            iyVal[l]=iyOff;
            ixVal[l]=(int) ((float)l * (float)(iDim) / (float)iDim) + 1 + iXpos;
        }
        iNum=0;
        iTimes=1;
        ddSum=0.0;
        ddCont=0.0;
        sCont  = new String("" + ddCont);
    }
    if(fCur > max)    fCur = max;
    if(fCur < min)    fCur = min;
    fVal = fCur;
    fLast = fCur;
    setLimits(min,max);
    cColValue = c;
    cColCur=c;
    sHead = new String(name);
    tfont = new Font("Dialog",Font.PLAIN,10);
    setFont(tfont);
}
/**
 * Initializes a meter canvas with new limits and store as default.
 * @param min parameter value range
 * @param max parameter value range
 */
public void setDefaultLimits(double min, double max){
defMin=min;
defMax=max;
setLimits(min,max);
}
/**
 * Initializes a meter canvas with new limits.
 * @param min parameter value range
 * @param max parameter value range
 */
public void setLimits(double min, double max){

    iInit=0;
    fMin  = min;
    fMax  = max;
    iMin = (int) min;
    iMax = (int) max;
    sMin  = new String("" + iMin);
    sMax  = new String("" + iMax);
    boolean log=isLog;
    createScale();
}
/**
 * Create a new meter from existing one. When clone shall be attached
 * to parent meter, typically these settings must be applied:<br> 
 * clone.setSizeXY(new Dimension(XSIZE_LARGE,YSIZE_LARGE));<br>
 * clone.hasParent();<br>
 * parent.setExternMeter(clone,Frame);
 * @param name Name of clone.
 */
public xMeter getClone(String name){
    xMeter extMeter=new xMeter(iMode, name, fMin, fMax, XSIZE, YSIZE, cColValue);
    extMeter.setAutoScale(boAutoScale);
    extMeter.setLogScale(isLog);
    extMeter.setLettering(sFullHead1,sFullHead2,sFullHead3,sUnits);
    extMeter.setColorBack(cColBack);
    return extMeter;
}
/**
 * Set some strings for lettering. In the compact mode Node:Appl and Name are shown in two lines.
 * Units are not shown. In large mode Node, Appl and Name are shown in three lines, units are shown.
 * @param node Node name. Get back by getHead1().
 * @param appl Application name. Get back by getHead2().
 * @param name Parameter name. Get back by getHead3().
 * @param units Units
 */
public void setLettering(String node, String appl, String name, String units){
    sUnits=units;
    sHead=node;
    sFullHead1=node;
    sFullHead2=appl;
    sFullHead3=name;
}
/**
 * Set units string.
 * @param units
 */
public void setUnits(String units){sUnits=units;}
/**
 * Set color of marker.
 * @param color
 */
public void setColor(Color color){cColValue=color;}
/**
 * Set back ground color. Color for text is two times brighter.
 * Background color should therefore be dark.
 * @param color
 */
public void setColorBack(Color color){cColBack=color;cColNorm=color.brighter().brighter();}
/**
 * Set color by name.
 * @param colorname (Red, Green, Blue, Yellow, Cyan, Magenta).
 */
public void setColor(String colorname){
    if(colorname.equals("Red"))         {sColor=colorname; cColValue=new Color(1.0f,0.0f,0.0f);}
    else if(colorname.equals("Blue"))   {sColor=colorname; cColValue=new Color(0.5f,0.5f,1.0f);}
    else if(colorname.equals("Green"))  {sColor=colorname; cColValue=new Color(0.0f,1.0f,0.0f);}
    else if(colorname.equals("Yellow")) {sColor=colorname; cColValue=new Color(1.0f,1.0f,0.0f);}
    else if(colorname.equals("Cyan"))   {sColor=colorname; cColValue=new Color(0.5f,1.0f,1.0f);}
    else if(colorname.equals("Magenta")){sColor=colorname; cColValue=new Color(1.0f,0.5f,1.0f);}
    cColCur=cColValue;
}
// from pull down menu.
private void askLimits(){
double low=fMin,high=fMax;
    boAutoScale=false;
    String str=JOptionPane.showInputDialog(this,"Enter limits: low,high",
        new String(Double.toString(fMin)+","+Double.toString(fMax)));
    if(str != null){
    String f[]=str.split(",");
    if(f.length == 1)high=Double.parseDouble(f[0]);
    if(f.length == 2){low=Double.parseDouble(f[0]);high=Double.parseDouble(f[1]);}
    setDefaultLimits(low,high);
    if(extMeter!=null)extMeter.setLimits(low,high);
}}
/**
 * If caller makes a clone of this Meter (parent), and wants that clone to be
 * controlled by this Meter (updated), it must set the clone to hasParent.
 * The clone is passed to its parent by setExternMeter together with a reference to
 * a JInternalFrame where the clone is displayed. This frame must be created by caller.
 * If frame is closed, the parent removes its child.
 */
protected void hasParent(){
boextMeter=true; // true: cannot create child
}
/**
 * Attach a second meter to this. Second meter will be updated from this.
 * @param extmeter Typically a clone with large size to be displayed in an extra frame.
 * @param extframe Extra frame for the clone. Must be handled by caller.
 */
protected void setExternMeter(xMeter extmeter, JInternalFrame extframe){
extMeter=extmeter;
extFrame=extframe;
boextMeter=true;
}
// If frame is detected to be gone, call this function to clean up.
private void removeMeter(){
extFrame=null;
extMeter=null;
boextMeter=false;
}
/**
 * Resets log scale, limits, autoscale, and mode to values as set by default settings.
 */
public void setDefaults(){
setLogScale(defLog);
setLimits(defMin,defMax);
setAutoScale(defAutoScale);
setMode(defMode);
}
public String getHead1(){return sFullHead1;}
public String getHead2(){return sFullHead2;}
public String getHead3(){return sFullHead3;}
public double getMin(){return fMin;}
public double getMax(){return fMax;}
public int getMode(){return iMode;}
public String getColor(){return sColor;}
public void setDefaultLogScale(boolean log){defLog=log;setLogScale(log);}
public void setLogScale(boolean log){isLog=log;createScale();}
public boolean getLogScale(){return isLog;}
public void setName(String sH){sHead = new String(sH);}
public void setDefaultAutoScale(boolean as){
defAutoScale=as;
setAutoScale(as);
}
public void setAutoScale(boolean as){
boAutoScale=as;
if(boAutoScale)fMax=fMin*1.1; // autoscale from low value
}
public boolean getAutoScale(){return boAutoScale;}
/**
 * Set update interval for trending and histogramming
 * @param seconds Time interval.
 */
public void setInterval(int seconds){iTimes=seconds;}
// xiPanelItem
public void setActionListener(ActionListener actionlistener){action=actionlistener;}
public JPanel getPanel() {return this;}
public String getName(){return sHead;}
public void setID(int i){iID=i;}
public int getID(){return iID;}
public Point getPosition(){return new Point(getX(),getY());};
public Dimension getDimension(){return new Dimension(ix,iy);};
public void setSizeXY(){setPreferredSize(new Dimension(ix,iy));}

public void setSizeXY(Dimension dd){
    ix=(int)dd.getWidth();
    iy=(int)dd.getHeight();
    iInit=0;
    iDim=0;
    imStore = null;
    createScale();
    clearStat();
    setMode(iMode);
    setPreferredSize(dd);
    repaint();
}
/**
 * Calculates new scaling (internally used).
 */
private void createScale(){
int ioff=iy-37; // was 38
    boLarge=iy>YSIZE;
    if(isLog){
        if((fMax-fMin)<0.1)fMax=fMin+0.1;
        if((fCur-fMin)<1.0)fCur=fMin+1.;
        if(boArc) fScale = 180.0 / Math.log10(fMax - fMin);
        if(boBar) fScaleBar = (double) (ix - 10) / Math.log10(fMax - fMin);
        if(boTrend) fScaleTrend = (double) (iy - ioff) / Math.log10(fMin - fMax);
        if(boStat) fScaleTrend = (double) (iy - ioff) / Math.log10(fMin - fMax);
        if(boArc) iAngleLast = (int) (Math.log10(fLast - fMin) * fScale);
        if(boBar) iAngleLast = (int) (Math.log10(fLast - fMin) * fScaleBar);
        double len=31.0;
        double fact=180.0 / Math.log10(180.);
        iytick[0]=(int)(Math.sin(Math.toRadians(Math.log10(45.)*fact))*len);// start with 30 for 6 intersections
        ixtick[0]=(int)(Math.cos(Math.toRadians(Math.log10(45.)*fact))*len);
        iytick[1]=(int)(Math.sin(Math.toRadians(Math.log10(90.)*fact))*len);
        ixtick[1]=(int)(Math.cos(Math.toRadians(Math.log10(90.)*fact))*len);
        iytick[2]=(int)(Math.sin(Math.toRadians(Math.log10(135.)*fact))*len);
        ixtick[2]=(int)(Math.cos(Math.toRadians(Math.log10(135.)*fact))*len);
    } else {
        if(boArc) fScale = 180.0 / (fMax - fMin);
        if(boBar) fScaleBar = (double) (ix - 10) / (fMax - fMin);
        if(boTrend) fScaleTrend = (double) (iy - ioff) / (fMin - fMax);
        if(boStat) fScaleTrend = (double) (iy - ioff) / (fMin - fMax);
        if(boArc) iAngleLast = (int) ((fLast - fMin) * fScale);
        if(boBar) iAngleLast = (int) ((fLast - fMin) * fScaleBar);
        double len=31.0;
        iytick[0]=(int)(Math.sin(Math.toRadians(45.))*len);
        ixtick[0]=(int)(Math.cos(Math.toRadians(45.))*len);
        iytick[1]=(int)(Math.sin(Math.toRadians(90.))*len);
        ixtick[1]=(int)(Math.cos(Math.toRadians(90.))*len);
        iytick[2]=(int)(Math.sin(Math.toRadians(135.))*len);
        ixtick[2]=(int)(Math.cos(Math.toRadians(135.))*len);
    }
}
/**
 * Clear trending and histogram arrays
 */
private void clearStat(){
int ioff=iy-37; // was 38
    iYpos=iy-49;
    if(boAutoScale)fMax=fMin+1.0;
    if(boTrend||boStat){
        iyOff=iy-18; // was 17+YSIZE-35=57, is YSIZE-18
        if(iDim==0){
            initMeter(sHead,fMin,fMax,ix,iy,cColValue);
        } else{
            for(int l=0;l<iDim;l++){
                fTrend[l]=0.0;
                fStat[l]=0.0;
                ddCont=0.0; // this will trigger to create iyVal from fStat new with next redraw
                iyVal[l]=iyOff;
                ixVal[l]=(int) ((float)l * (float)(iDim) / (float)iDim) + 1 + iXpos;
            }
            iYpos = iy-58; // was 17
            setLimits(fMin,fMax);
        }
    }
}
/**
 * Set mode of presentation.
 * @param mode ARC, BAR, TREND, STAT
 */
public void setDefaultMode(int mode){
defMode=mode;
setMode(mode);
}
/**
 * Set mode of presentation.
 * @param mode ARC, BAR, TREND, STAT
 */
public void setMode(int mode){
    iMode=mode;
    boArc = false;
    boBar = false;
    boTrend=false;
    boStat =false;
    iDim=0;
    if(mode == ARC){
        boArc = true;
        iYpos=iy-49;
        setLimits(fMin,fMax);
    } else if(mode==BAR){
        boBar = true;
        iYpos=iy-49;
        setLimits(fMin,fMax);
    } else if(mode==STAT){
        boStat = true;
        if(iDim==0){
            initMeter(sHead,fMin,fMax,ix,iy,cColValue);
        } else{
            for(int l=0;l<iDim;l++){
                // fTrend[l]=0.0;
                // fStat[l]=0.0;
                ddCont=0.0; // this will trigger to create iyVal from fStat new with next redraw
                iyVal[l]=iyOff;
                ixVal[l]=(int) ((float)l * (float)(iDim) / (float)iDim) + 1 + iXpos;
            }
            iYpos = iy-58; // was 17
            setLimits(fMin,fMax);
        }
    } else if(mode==TREND){
        boTrend = true;
        if(iDim==0){
            initMeter(sHead,fMin,fMax,ix,iy,cColValue);
        } else{
            for(int l=0;l<iDim;l++){
                // fTrend[l]=0.0;
                // fStat[l]=0.0;
                ddCont=0.0; // this will trigger to create iyVal from fStat new with next redraw
                iyVal[l]=iyOff;
                ixVal[l]=(int) ((float)l * (float)(iDim) / (float)iDim) + 1 + iXpos;
            }
            iYpos = iy-58; // was 17
            setLimits(fMin,fMax);
        }
    }
}
/**
 * Toggle between meter and bar mode. If mode was none of both, meter is set.
 * @param meter True: select meter, false: bar.
 */
private void toggleMeter(boolean meter){
    boArc = meter;
    boBar = !meter;
    if(boArc)iMode=ARC;
    if(boBar)iMode=BAR;
    defMode=iMode;
    boTrend=false;
    boStat=false;
    iYpos=iy-49;
    setLimits(fMin,fMax);
    repaint();
}
/**
 * Toggle between trend mode and histogram mode. If mode was none of both, histogram is set.
 * @param stat True: select histogram, false: trend.
 */
private void toggleTrend(boolean stat){
    boStat = stat;
    boTrend = !stat;
    if(boStat)iMode=STAT;
    if(boTrend)iMode=TREND;
    defMode=iMode;
    boArc=false;
    boBar=false;
    if(iDim==0){
        initMeter(sHead,fMin,fMax,ix,iy,cColValue);
    } else{
        for(int l=0;l<iDim;l++){
            // fTrend[l]=0.0;
            // fStat[l]=0.0;
            ddCont=0.0; // this will trigger to create iyVal from fStat new with next redraw
            iyVal[l]=iyOff;
            ixVal[l]=(int) ((float)l * (float)(iDim) / (float)iDim) + 1 + iXpos;
        }
        iYpos = iy-58; // was 17
        setLimits(fMin,fMax);
    }
    repaint();
}
/**
 * Called by repaint, calls update.
 */
public void paintComponent(Graphics g){
// draw image
// System.out.println("paint");
    if(imStore == null){
        imStore = new BufferedImage(ix,iy,BufferedImage.TYPE_INT_RGB);
        gg = imStore.createGraphics();
        gg.addRenderingHints(renderHint);
    }
    update(g); // update is not called by repaint!
    Graphics2D g2d = (Graphics2D)g;
    g2d.drawImage(imStore,new AffineTransform(1f,0f,0f,1f,0,0),this);
}
/**
 * Overwriting update method we avoid clearing the
 * graphics. This would cause flickering.
 * Update is not called by repaint!
 */
public void update(Graphics g){
// static graphics into memory image
    int ixtemp;
    if(iInit == 0){
        if(imStore == null){
            imStore = new BufferedImage(ix,iy,BufferedImage.TYPE_INT_RGB);
            gg = imStore.createGraphics();
            gg.addRenderingHints(renderHint);
        }
        fm = gg.getFontMetrics();
        iInit = 1;
    } // init
    iHead = (ix - fm.stringWidth(sHead)) / 2;
    iMinL  = fm.stringWidth(sMin);
    iMaxL  = fm.stringWidth(sMax);
    iUnitL = fm.stringWidth(sUnits);
    if((fVal > fMax) || (fVal < fMin))
        gg.setColor(cColAlert);
    else gg.setColor(cColNorm);
    gg.fillRect(0,0,ix,iy);
    gg.setColor(cColBack);
    gg.fillRect(1,1,ix-2,iy-2);
    if(boArc){
        iYpos=iy-46; // , upper left corner of arc, was 29 for standard YSIZE ys
        //****gg.setColor(cColValue);
        gg.setColor(cColText); // outer circle
        gg.fillArc(18,iYpos-1,64,64,-180,-180);
        gg.setColor(Color.lightGray); // outer circle
        gg.fillArc(19,iYpos,62,62,-180,-180);
        gg.setColor(cColBack); // inner circle
        //****gg.fillArc(19,iYpos,62,62,-180,-180);
        gg.fillArc(20,iYpos+1,60,60,-180,-180);
    }
    if(boBar){
        gg.setColor(cColText); 
        if(isLog) {
            gg.drawLine(5,iYpos+11,5,iYpos+27);
            ixtemp=5+(int)(Math.log10(fMax/4. - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)(Math.log10(fMax/2. - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)(Math.log10(fMax*0.75 - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)(Math.log10(fMax - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
        } else {
            gg.drawLine(5,iYpos+11,5,iYpos+27);
            ixtemp=5+(int)((fMax/4. - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)((fMax/2. - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)((fMax*0.75 - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
            ixtemp=5+(int)((fMax - fMin) * fScaleBar);
            gg.drawLine(ixtemp,iYpos+11,ixtemp,iYpos+27);
        }
    }
    if(boTrend){
        gg.setColor(cColText);
        gg.drawRect(iXpos,iYpos,ix-iXpos-6,41); // 41=iy-34
        gg.drawString("Time",ix/2,iy-4);
    }
    if(boStat){
        gg.setColor(cColText);
        gg.drawRect(iXpos,iYpos,ix-iXpos-6,41); // 41=iy-34
        gg.drawString(sMin,iXpos,iy-4);
        gg.drawString(sMax,ix-3-iMaxL,iy-4);
    }
    gg.setColor(cColHead);
    if(boLarge){
        gg.drawString(sFullHead1,10,14);
        gg.drawString(sFullHead2,10,28);
        gg.drawString(sFullHead3,10,42);
    } else {
        gg.drawString(sFullHead1+":"+sFullHead2,5,14);
        gg.drawString(sFullHead3,5,28);
    }
    if(boArc || boBar){
        gg.setColor(cColText);
        gg.drawString(sMin,5,iy-4);
        if(boLarge) {
            int i = ix-6-iMaxL-iUnitL;
            if(boArc && i>80) i=80;
            gg.drawString(new String(sMax+" "+sUnits),i,iy-4);
        } else
        gg.drawString(sMax,ix-3-iMaxL,iy-4);
        sCur  = new String(String.format("%6.1f",fVal));
        if(sCur.length() > 10)
            try{sCur = sCur.substring(0,9);} catch(StringIndexOutOfBoundsException e){}
        iCurL  = fm.stringWidth(sCur);
        gg.setColor(cColValue);
        if(boLarge)gg.drawString(sCur,ix-iCurL-3,56);
        else       gg.drawString(sCur,ix-iCurL-3,41);
        if(boArc) {
            if(isLog) {
                if((fCur-fMin)<1.0) fCur=fMin+1.0;
                fAngle = (Math.log10(fCur - fMin) * fScale);
            } else  fAngle = ((fCur - fMin) * fScale);
            iAngle=(int)fAngle;
            gg.setColor(cColNorm);
            gg.fillArc(21,iYpos+2,58,58,-180,-iAngleLast);
            int ixcos=(int)(Math.cos(Math.toRadians(fAngle))*29.);
            int iysin=(int)(Math.sin(Math.toRadians(fAngle))*29.);
            gg.setColor(Color.lightGray);
            int rad=6;
            gg.drawLine(50-ixtick[0],iYpos+31-iytick[0],50,iYpos+30);
            gg.drawLine(50-ixtick[1],iYpos+31-iytick[1],50,iYpos+30);
            gg.drawLine(50-ixtick[2],iYpos+31-iytick[2],50,iYpos+30);
            gg.setColor(cColValue);
            ixpol[0]=50-ixcos;iypol[0]=iYpos+30-iysin;
            ixpol[1]=48;iypol[1]=iYpos+30;
            ixpol[2]=52;iypol[2]=iYpos+30;
            if(iAngle<60)iypol[2]-=2;
            if(iAngle<30)iypol[2]-=2;
            if(iAngle>120)iypol[1]-=1;
            if(iAngle>150)iypol[1]-=1;
            gg.fillPolygon(ixpol,iypol,3);
            gg.setColor(cColText); // bottom line
            gg.fillRect(20,iYpos+29,62,2); // bottom line
            gg.setColor(cColText); // bottom line
            gg.fillArc(44,iYpos+25,12,12,-180,-180);
            gg.setColor(Color.lightGray); // bottom line
            gg.fillRect(20,iYpos+30,62,1); // bottom line
        }
        if(boBar){
            if(isLog) {
                if((fCur-fMin)<1.0) fCur=fMin+1.0;
                fAngle = (Math.log10(fCur - fMin) * fScaleBar);
            } else  fAngle = ((fCur - fMin) * fScaleBar);
            iAngle=(int)fAngle;
            gg.setColor(cColNorm);
            gg.fillRect(6,iYpos+14,iAngleLast,11);
            gg.setColor(cColText);
            gg.fillRect(6,iYpos+14,iAngle,11);
            gg.setColor(cColNosp);
            gg.fillRect(8,iYpos+16,iAngle-2,9);
            gg.setColor(cColValue);
            gg.fillRect(8,iYpos+16,iAngle-4,7);
        }
    } // arc or bar
    if(boTrend){
        gg.setColor(cColValue);
        gg.fillPolygon(ixVal,iyVal,iDim);
    }
    if(boStat){
        iyVal[0]=iyOff;
        iyVal[iDim-1]=iyOff;
        gg.setColor(cColValue);
        gg.drawPolygon(ixVal,iyVal,iDim);
    }
    if(boStat||boTrend){
        // save current transform
        AffineTransform af = gg.getTransform();
        gg.rotate(-Math.PI/2.0); // rotate left 90
        gg.translate(-iy,0);
        // whole image is rotated around upper left corner
        // Therefore we must pull down y
        gg.setColor(cColText);
        if(boStat)gg.drawString(sCont,20,iXpos-2);
        if(boTrend)gg.drawString(sMin+":"+sMax,5,iXpos-2);
        //if(boTrend)gg.drawString(sMax,27,iYpos-2);
        // restore transform
        gg.setTransform(af);
    }
//paint(g);
}
/**
 * Redraw without changes.
 */
public void redraw(){redraw(fMin);}
/**
 * Redraw with new value.
 * @param value New value
 * @param valid True if value is valid, false if not. Change color on change in validity.
 * @param draw True: call redraw, otherwise no repaint.
 */
public void redraw(double value, boolean valid, boolean draw){
if(valid){
    if(!valueValid){
    valueValid=true;
    cColValue=cColCur;
    }
}else{
    if(valueValid){
    valueValid=false;
    cColValue=cColNorm;
}}
if(draw)redraw(value);
}
/**
 * Final redraw with new value. Called by all other redraw entries.
 * @param value New value.
 */
public void redraw(double value){
int ioff=iy-37; // was 38
    if(boAutoScale && (value > fMax)){
        double Min=fMin;
        int dec = (int)Math.log10(2.0*value);
        fMax=Math.pow(10.,(double)dec);
        double Max=fMax;
        while(value>fMax)fMax+=Max;
        Max=2.0*fMax;
        //  System.out.println("rescale max "+fMax);
        setLimits(Min,Max);
        if(boStat||boTrend)for(int l=0;l<iDim;l++){
            ddSum=0.0;
            ddCont=0.0;
            fTrend[l]=0.0;
            fStat[l]=0.0;
            iyVal[l]=iyOff;
        }
    } // rescale
    if(boTrend){
        ddSum=ddSum+value;
        if(this.isShowing())iNum=iNum+1;
        if(iNum >= iTimes){
            ddSum=ddSum/(double)iNum;
            iNum=0;
            fCur = ddSum;
            fVal = ddSum;
            if(ddSum > fMax){
                fMax=ddSum*1.2;
                fScaleTrend = (double) (iy - ioff) / (fMin - fMax);
                for(int i=1;i<iDim-2;i++){
                    fTrend[i]=fTrend[i+1];
                    iyVal[i]=iyOff + (int) ((fTrend[i]-fMin)*fScaleTrend);
                }
                iMin = (int) fMin;
                iMax = (int) fMax;
                sMin  = new String("" + iMin);
                sMax  = new String("" + iMax);
            } else for(int i=1;i<iDim-2;i++){
                iyVal[i]=(int) iyVal[i+1];
                fTrend[i]=fTrend[i+1];
            }
            iyVal[iDim-2]=iyOff + (int) ((ddSum-fMin) * fScaleTrend);
            fTrend[iDim-2]=ddSum;
            fLast = fCur;
            ddSum=0.0;
            // fill statistics anyway
            i=(int)((value-fMin)/(fMax-fMin)*(double)iDim);
            if(i<0)i=0;
            if(i>=iDim)i=iDim-1;
            fStat[i]+=1.0;
            repaint();
        }
    }
    if(boStat){
        if(this.isShowing())iNum=iNum+1;
        i=(int)((value-fMin)/(fMax-fMin)*(double)iDim);
        if(i<0)i=0;
        if(i>=iDim)i=iDim-1;
        fStat[i]+=1.0;
        if(fStat[i]>ddCont){
            ddCont=2.0*fStat[i]; // rescale
            for(i=0;i<iDim;i++)  iyVal[i]=iyOff - (int) (fStat[i]*(double) (iy - ioff) /ddCont);
            //System.out.println("rescale cont "+ddCont);
            iInit=0; // signal refresh
        } else iyVal[i]=iyOff - (int) (fStat[i]*(double) (iy - ioff) /ddCont);
        // dont use i here anymore!
        sCont  = new String("" + (int)ddCont);
        if(iNum >= iTimes){
            iNum=0;
            repaint();
        }
    }
    if(boArc || boBar){
        fCur = value;
        fVal = value;
        if(value > fMax){
            fCur = fMax;
        }
        if(value < fMin){
            if(isLog) fCur = fMin+1.0;
            else fCur = fMin;
        }
        if(fCur > fLast){
            fLast = fCur;
            double s = fScale;
            if(boBar) s = fScaleBar;
            if(isLog) iAngleLast = (int) (Math.log10(fCur - fMin) * s);
            else iAngleLast = (int) ((fCur - fMin) * s);
        }
        repaint();
    }
    if(extFrame != null){
        if(extFrame.isClosed()){
            extFrame=null;
            removeMeter();
        }
    } else removeMeter();
    if(extMeter!=null)extMeter.redraw(value);
    }
// Context menu execution (ActionListener)
/**
 * Dispatch actions from right mouse button.
 */
public void actionPerformed(ActionEvent a){
    String c=a.getActionCommand();
    setColor(c); // only valid color names are applied.
    if(c.equals(sHead));
    if(c.equals("Meter"))toggleMeter(true);
    if(c.equals("Bar"))toggleMeter(false);
    if(c.equals("Statistics"))toggleTrend(true);
    if(c.equals("Trend"))toggleTrend(false);
    if(c.equals("setLin"))setDefaultLogScale(false);
    if(c.equals("setLog"))setDefaultLogScale(true);
    if(c.equals("setFixscale"))setDefaultAutoScale(false);
    if(c.equals("setAutoscale"))setDefaultAutoScale(true);
    if(c.equals("Limits"))askLimits();
    if(c.equals("Clear"))clearStat();
    if(c.equals("Large"))
        if((action!=null)&(!boextMeter)){
            boextMeter=true;
            action.actionPerformed(new ActionEvent(this,getID(),"Large"));
            }
    if(c.equals("Discard"));
}
// context menu definition (MouseListener)
/**
 * Pull down context menu with right mouse button
 */
public void mousePressed(MouseEvent me) {
    if(me.getButton()==MouseEvent.BUTTON3){
        JPopupMenu men=new JPopupMenu();
        if(!boLarge){
            men.add(new JMenuItem(sFullHead1));
            men.add(new JMenuItem(sFullHead2));
            men.add(new JMenuItem(sFullHead3));
            men.add(new JMenuItem(sUnits));
        }
        //menit.addActionListener(this);
        men.addSeparator();
        if(!boArc){
            menit=new JMenuItem("Meter");
            men.add(menit);
            menit.addActionListener(this);
        }
        if(!boBar){
            menit=new JMenuItem("Bar");
            men.add(menit);
            menit.addActionListener(this);
        }
        if(!boStat){
            menit=new JMenuItem("Statistics");
            men.add(menit);
            menit.addActionListener(this);
        }
        if(!boTrend){
            menit=new JMenuItem("Trend");
            men.add(menit);
            menit.addActionListener(this);
        }
        men.addSeparator();
        if(boAutoScale) menit=new JMenuItem("setFixscale");
        else menit=new JMenuItem("setAutoscale");
        men.add(menit);
        menit.addActionListener(this);
        if(isLog) menit=new JMenuItem("setLin");
        else menit=new JMenuItem("setLog");
        men.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Limits");
        men.add(menit);
        menit.addActionListener(this);
        menit=new JMenuItem("Clear");
        men.add(menit);
        menit.addActionListener(this);
        men.addSeparator();
        JMenu colmen=new JMenu("Colors");
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
        if(!boextMeter){
            menit=new JMenuItem("Large");
            men.add(menit);
            menit.addActionListener(this);
        }
        // menit=new JMenuItem("Discard");
        // men.add(menit);
        // menit.addActionListener(this);
        men.setLightWeightPopupEnabled(false); // makes popup medium weight to go on top of heavy weight canvas
        men.setVisible(true);
        men.show(me.getComponent(),me.getX(),me.getY());
    }
}
/**
 * Not used.
 */
public void mouseClicked(MouseEvent me){}
/**
 * Not used.
 */
public void mouseEntered(MouseEvent me){}
/**
 * Not used.
 */
public void mouseExited(MouseEvent me){}
/**
 * Not used.
 */
public void mouseReleased(MouseEvent me){}
}
