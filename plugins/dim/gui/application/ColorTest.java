import xgui.*;
import java.awt.*;
import java.awt.image.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.JMenuItem;
import java.awt.geom.AffineTransform;

public class ColorTest extends JPanel 
{
public final static int XSIZE=400;
public final static int YSIZE=200;
private  int ix, iy, x, y, w, h, iInit;
private FontMetrics fm;
private Font tfont;
private BufferedImage imStore;
private Graphics2D gg;

public ColorTest(){
ix=800;
iy=330;
w=50; // width color column
h=14; // height color line
    setSize(ix,ix);
    setPreferredSize(new Dimension(ix,iy));
    tfont = new Font("Dialog",Font.PLAIN,10);
    setFont(tfont);
}

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
    // backgrounds
    x=0;
    gg.setColor(xSet.white());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.grayL());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.yellowL());  gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.greenL());   gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.blueL());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.redL());     gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.cyanL());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.magentaL()); gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.grayD());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.yellowD());  gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.greenD());   gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.blueD());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.redD());     gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.cyanD());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.magentaD()); gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    gg.setColor(xSet.black());    gg.fillRect(x*w,0,(x+1)*w,iy); x++;
    for(x=0;x<16;x++){
    y=1;
    gg.setColor(xSet.white());    gg.drawString("white",x*w+2,y*h-2);y++;
    gg.setColor(xSet.grayL());    gg.drawString("grayL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.yellowL());  gg.drawString("yellL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.greenL());   gg.drawString("greenL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.blueL());    gg.drawString("blueL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.redL());     gg.drawString("redL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.cyanL());    gg.drawString("cyanL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.magentaL()); gg.drawString("magL",x*w+2,y*h-2);y++;
    gg.setColor(xSet.gray());    gg.drawString("gray",x*w+2,y*h-2);y++;
    gg.setColor(xSet.yellow());  gg.drawString("yell",x*w+2,y*h-2);y++;
    gg.setColor(xSet.green());   gg.drawString("green",x*w+2,y*h-2);y++;
    gg.setColor(xSet.blue());    gg.drawString("blue",x*w+2,y*h-2);y++;
    gg.setColor(xSet.red());     gg.drawString("red",x*w+2,y*h-2);y++;
    gg.setColor(xSet.cyan());    gg.drawString("cyan",x*w+2,y*h-2);y++;
    gg.setColor(xSet.magenta()); gg.drawString("mag",x*w+2,y*h-2);y++;
    gg.setColor(xSet.grayD());    gg.drawString("grayD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.yellowD());  gg.drawString("yellD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.greenD());   gg.drawString("greenD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.blueD());    gg.drawString("blueD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.redD());     gg.drawString("redD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.cyanD());    gg.drawString("cyanD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.magentaD()); gg.drawString("magD",x*w+2,y*h-2);y++;
    gg.setColor(xSet.black());    gg.drawString("black",x*w+2,y*h-2);y++;
    }
}

public void redraw(){repaint();}

}
