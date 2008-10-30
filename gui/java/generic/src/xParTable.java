package xgui;

import java.util.Vector;
import java.util.Comparator;
import java.util.Date;
import java.util.Collections;
import javax.swing.table.TableModel;
import javax.swing.event.TableModelEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.InputEvent;
import javax.swing.JTable;
import javax.swing.table.DefaultTableModel;
import javax.swing.event.TableModelListener;
import javax.swing.table.TableColumnModel;
import javax.swing.table.JTableHeader;

public class xParTable extends DefaultTableModel implements Comparator{

protected   int         currCol, editCol, meterCol;
protected   Vector      ascendCol;  // this vector stores the info (ascending or descending) of each column
protected   Integer     one         = new Integer(1);
protected   Integer     minusOne    = new Integer(-1);

public xParTable(int edit, int meter) {
    super();
    editCol=edit;
    meterCol=meter;
    ascendCol = new Vector();
}

/*******************************************************************
 * addColumn methods are inherited from the DefaultTableModel class.
 *******************************************************************/

public void addColumn(Object columnName) {
    super.addColumn(columnName);
    ascendCol.add(one);
}

public void addColumn(Object columnName, Object[] columnData) {
    super.addColumn(columnName, columnData);
    ascendCol.add(one);
}

public void addColumn(Object columnName, Vector columnData) {
    super.addColumn(columnName, columnData);
    ascendCol.add(one);
}

public Class getColumnClass(int c) {

    return getValueAt(0,c).getClass();
}
/*****************************************************************
 * This method is the implementation of the Comparator interface.
 * It is used for sorting the rows
 *****************************************************************/
public int compare(Object v1, Object v2) {

    // the comparison is between 2 vectors, each representing a row
    // the comparison is done between 2 objects from the different rows that are in the column that is being sorted

    int ascending = ((Integer) ascendCol.get(currCol)).intValue();
    if (v1 == null && v2 == null) {
        return 0;
    } else if (v1 == null) { // Define null less than everything.
        return -1 * ascending;
    } else if (v2 == null) {
        return 1 * ascending;
    }

    Object o1 = ((Vector) v1).get(currCol);
    Object o2 = ((Vector) v2).get(currCol);

    Class type;
    if(o1 != null)
        type = o1.getClass();
    else if(o2 != null)
        type = o2.getClass();
    else
        type = java.lang.Object.class;

    // If both values are null, return 0.
    if (o1 == null && o2 == null) {
        return 0;
    } else if (o1 == null) { // Define null less than everything.
        return -1 * ascending;
    } else if (o2 == null) {
        return 1 * ascending;
    }


    if (type.getSuperclass() == java.lang.Number.class) {
        Number n1 = (Number) o1;
        double d1 = n1.doubleValue();
        Number n2 = (Number) o2;
        double d2 = n2.doubleValue();

        if (d1 < d2) {
            return -1 * ascending;
        } else if (d1 > d2) {
            return 1 * ascending;
        } else {
            return 0;
        }
    } else if (type == java.util.Date.class) {
        Date d1 = (Date) o1;
        long n1 = d1.getTime();
        Date d2 = (Date) o2;
        long n2 = d2.getTime();

        if (n1 < n2) {
            return -1 * ascending;
        } else if (n1 > n2) {
            return 1 * ascending;
        } else {
            return 0;
        }
    } else if (type == String.class) {
        String s1 = (String) o1;
        String s2    = (String) o2;
        int result = s1.compareTo(s2);

        if (result < 0) {
            return -1 * ascending;
        } else if (result > 0) {
            return 1 * ascending;
        } else {
            return 0;
        }
    } else if (type == Boolean.class) {
        Boolean bool1 = (Boolean) o1;
        boolean b1 = bool1.booleanValue();
        Boolean bool2 = (Boolean) o2;
        boolean b2 = bool2.booleanValue();

        if (b1 == b2) {
            return 0;
        } else if (b1) { // Define false < true
            return 1 * ascending;
        } else {
            return -1 * ascending;
        }
    } else {
        String s1 = o1.toString();
        String s2 = o2.toString();
        int result = s2.compareTo(s1);

        if (result < 0) {
            return -1 * ascending;
        } else if (result > 0) {
            return 1 * ascending;
        } else {
            return 0;
        }
    }
}

/***************************************************************************
 * This method sorts the rows using Java's Collections class.
 * After sorting, it changes the info of the column -
 * if the column was ascending, its new info is descending, and vice versa.
 ***************************************************************************/
public void sort() {
    Collections.sort(dataVector, this);
    Integer val = (Integer) ascendCol.get(currCol);
    ascendCol.remove(currCol);
    if(val.equals(one)) // change the info of the column
        ascendCol.add(currCol, minusOne);
    else
        ascendCol.add(currCol, one);
}

public void sortByColumn(int column) {
    this.currCol = column;
    sort();
    fireTableChanged(new TableModelEvent(this));
}


// Add a mouse listener to the Table to trigger a table sort
// when a column heading is clicked in the JTable.
public void addMouseListenerToHeaderInTable(JTable table) {
    final xParTable sorter = this;
    final JTable tableView = table;
    //tableView.setColumnSelectionAllowed(false);
    MouseAdapter listMouseListener = new MouseAdapter() {
        public void mouseClicked(MouseEvent e) {
            TableColumnModel columnModel = tableView.getColumnModel();
            int viewColumn = columnModel.getColumnIndexAtX(e.getX());
            int column = tableView.convertColumnIndexToModel(viewColumn);
            if (e.getClickCount() == 1 && column != -1) {
                int shiftPressed = e.getModifiers()&InputEvent.SHIFT_MASK;
                boolean ascending = (shiftPressed == 0);
                sorter.sortByColumn(column);
            }
        }
    };
    JTableHeader th = tableView.getTableHeader();
    th.addMouseListener(listMouseListener);
}

public boolean isCellEditable( int r, int c ) {
    if ( c == editCol ) return true;
    if ( c == meterCol ) return true;
    return false;

}

public Integer getAscendingOrder(int c) {
    Integer val = (Integer) ascendCol.get(c);
    return val;
}
}

