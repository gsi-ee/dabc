package xgui;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.ImageIcon;
import javax.swing.JPanel;
import javax.swing.JDesktopPane;
import javax.swing.JSplitPane;

public class xInternalCompound extends xInternalFrame
{
private ImageIcon menuIcon;
private ActionListener action;
private JPanel frapan;
private int topdiv, bottomdiv;
private int dividerSize;
private Color backColor;

public xInternalCompound(String title, ImageIcon icon, Point divisions, xLayout la, Color back){
    super(title,la);
    JSplitPane sp=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    dividerSize=sp.getDividerSize();
    menuIcon=icon;
    topdiv=(int)divisions.getX();
    bottomdiv=(int)divisions.getY();
    if(back == null) backColor = xSet.getColorBack();
    else backColor=back;
}
public void rebuild(JPanel panel1, JPanel panel2, JPanel panel3){
    JPanel frapan=new JPanel();
    JSplitPane hori=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    JSplitPane verti=new JSplitPane(JSplitPane.VERTICAL_SPLIT);
    hori.setOneTouchExpandable(true);
    verti.setOneTouchExpandable(true);
    frapan.setBackground(backColor);
    hori.setBackground(backColor);
    verti.setBackground(backColor);
    // left 1, right top 2, right bottom 3
    if(topdiv == 0){
        verti.setTopComponent(panel2);
        verti.setBottomComponent(panel3);
        verti.getBottomComponent().setBackground(backColor);
        verti.getTopComponent().setBackground(backColor);
        hori.setLeftComponent(panel1);
        hori.setRightComponent(verti);
        hori.getLeftComponent().setBackground(backColor);
        hori.setDividerLocation(-1); // adjust according left
        frapan.add(hori);
    // top 1, bottom left 2, bottom right 3
    } else if(topdiv == 1){
        hori.setLeftComponent(panel2);
        hori.setRightComponent(panel3);
        hori.getLeftComponent().setBackground(backColor);
        hori.getRightComponent().setBackground(backColor);
        hori.setDividerLocation(-1); // adjust according left
        verti.setBottomComponent(hori);
        verti.setTopComponent(panel1);
        verti.getTopComponent().setBackground(backColor);
        frapan.add(verti);
    // top left 1, top right 2, bottom 3
    } else if(topdiv == 2){
        hori.setLeftComponent(panel1);
        hori.setRightComponent(panel2);
        hori.getLeftComponent().setBackground(backColor);
        hori.getRightComponent().setBackground(backColor);
        hori.setDividerLocation(-1); // adjust according left
        verti.setTopComponent(hori);
        verti.setBottomComponent(panel3);
        verti.getBottomComponent().setBackground(backColor);
        verti.getTopComponent().setBackground(backColor);
        frapan.add(verti);
    }
    setupFrame(menuIcon, null, frapan, true);  // true for resizable
}
public void rebuild(JPanel panel1, JPanel panel2){
    JPanel frapan=new JPanel();
    JSplitPane hori=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    JSplitPane verti=new JSplitPane(JSplitPane.VERTICAL_SPLIT);
    hori.setOneTouchExpandable(true);
    verti.setOneTouchExpandable(true);
    frapan.setBackground(backColor);
    hori.setBackground(backColor);
    verti.setBackground(backColor);
    // left 1, right 2
    if(topdiv==0){
        hori.setLeftComponent(panel1);
        hori.setRightComponent(panel2);
        hori.setDividerLocation(-1); // adjust according left
        frapan.add(hori);
    // top 1, bottom 2
    } else {
        verti.setTopComponent(panel1);
        verti.setBottomComponent(panel2);
        frapan.add(verti);
    }
    setupFrame(menuIcon, null, frapan, true);  // true for resizable
}    
public void rebuild(JPanel panel){
    setupFrame(menuIcon, null, panel, true);  // true for resizable
}
public void rebuild(JPanel panel1, JPanel panel2, JPanel panel3, JPanel panel4){
    JPanel frapan=new JPanel();
    JSplitPane hori1=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    JSplitPane hori2=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
    JSplitPane verti=new JSplitPane(JSplitPane.VERTICAL_SPLIT);
    hori1.setOneTouchExpandable(true);
    hori2.setOneTouchExpandable(true);
    verti.setOneTouchExpandable(true);
    frapan.setBackground(backColor);
    hori1.setBackground(backColor);
    hori2.setBackground(backColor);
    verti.setBackground(backColor);
    hori1.setLeftComponent(panel1);
    hori1.setRightComponent(panel2);
    hori1.setDividerLocation(-1); // adjust according left
    hori2.setLeftComponent(panel3);
    hori2.setRightComponent(panel4);
    hori2.setDividerLocation(-1); // adjust according left
    verti.setTopComponent(hori1);
    verti.setBottomComponent(hori2);
    frapan.add(verti);
    setupFrame(menuIcon, null, frapan, true);  // true for resizable
}
public int getDividerSize(){return dividerSize;}

}