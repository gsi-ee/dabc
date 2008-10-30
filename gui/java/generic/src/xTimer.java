package xgui;
/**
*
* @author goofy
*/
import javax.swing.Timer;
import java.awt.event.*;
import java.util.*;
/**
*/
public class xTimer extends Timer{
public xTimer(ActionListener a, boolean repeat){
    super(1,a);
    setRepeats(repeat);
}
public void action(ActionEvent aev){
fireActionPerformed(aev);
}
}
