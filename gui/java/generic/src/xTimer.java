package xgui;
import javax.swing.Timer;
import java.awt.event.*;
import java.util.*;

/**
 * Timer class to launch actions.
 * @author Hans G. Essel
 * @version 1.0
 */
public class xTimer extends Timer{
/**
 * Constructor of xTimer.
 * @param actlis Passed to Timer.
 * @param repeat True: repeat firing the timer, false: only once 
 */
public xTimer(ActionListener actlis, boolean repeat){
    super(1,actlis);
    setRepeats(repeat);
}
/**
 * Call fireActionPerformed function of Timer (could be called directly instead).
 * @param aev Event passed to the fireActionPerformed function of Timer.
 */
public void action(ActionEvent aev){
fireActionPerformed(aev);
}
}
