package runtime;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * Author: Diego Didona Email: didona@gsd.inesc-id.pt Websiste: www.cloudtm.eu Date: 24/05/12
 */
public abstract class OutputParser<T> {

   private Translation translation;
   private String[][] stats;
   private int numColumns;
   private int numRows;
   private final static Log log = LogFactory.getLog(OutputParser.class);
   private final String sep;
   private final static boolean trace = log.isTraceEnabled();

   private boolean filterLines = false;

   public void setFilterLines(boolean filterLines) {
      this.filterLines = filterLines;
   }

   protected List<T> column(String param) throws ParameterNotFoundException {
      final List<T> ret = new ArrayList<>();
      final int id = translation.getParamIndex(param);
      for (String[] stat : stats) {
         ret.add(_getParam(stat[id]));
      }
      return ret;
   }

   public String columnId(int column) {
      return translation.headerEntryFor(column);
   }

   public List<String> rawColumn(String param) throws ParameterNotFoundException {
      final LinkedList<String> ret = new LinkedList<>();
      final int id = translation.getParamIndex(param);
      for (String[] stat : stats) {
         ret.addLast(stat[id]);
      }
      return ret;
   }


   public String[] rawRow(String headerEntry, String id) throws ParameterNotFoundException {
      int rowIndex = idWithStringColumn(headerEntry, id);
      if (rowIndex == -1)
         return null;
      return rawRow(rowIndex);
   }

   public String[] rawRow(int rowIndex) throws ParameterNotFoundException {
      String[] toRet = new String[numColumns()];
      System.arraycopy(stats[rowIndex], 0, toRet, 0, toRet.length);
      return toRet;
   }

   protected List<T> column(int paramIndex) {
      final List<T> ret = new ArrayList<>();
      for (String[] stat : stats) {
         ret.add(_getParam(stat[paramIndex]));
      }
      return ret;
   }

   /**
    * Copy a row, excluding the entry corresponding to the headerEntry passed as input
    *
    * @param headerEntry
    * @param id
    * @return
    * @throws ParameterNotFoundException
    */
   public String[] rawRowExcludingTargetEntry(String headerEntry, String id) throws ParameterNotFoundException {
      int rowIndex = idWithStringColumn(headerEntry, id);
      if (rowIndex == -1)
         return null;
      String[] toRet = new String[numColumns() - 1];
      int indexForHeaderEntry = this.translation.indexForHeaderEntry(headerEntry);
      for (int i = 0, j = 0; i < numColumns(); i++) {
         if (i != indexForHeaderEntry) {
            toRet[j] = stats[rowIndex][i];
            j++;
         }
      }
      return toRet;
   }

   public String headerIfFor(int columnId) {
      return translation.headerEntryFor(columnId);
   }

   public OutputParser(String filePath, String sep) throws IOException {
      this.sep = sep;
      BufferedReader source = new BufferedReader(new FileReader(new File(filePath)));

      String header = source.readLine();
      this.translation = new Translation(header, sep);

      this.numColumns = translation.size();
      this.numRows = getNumRows(filePath);
      if (trace) log.trace("Total number of rows is " + numRows);

      String[][] temp = new String[numRows][numColumns];
      this.initializeStats(source, temp);
      stats = temp;
      source.close();

   }

   private void initializeStats(BufferedReader br, String[][] stats) throws IOException {
      String row;
      int i = 0;
      //br has not the header
      while ((row = br.readLine()) != null) {
         if (fillRow(stats, i, row)) {
            i++;
         }
      }
      if (trace) log.trace("Rows " + numRows + " columns " + numColumns);
      //dump((Double[][])stats,stats.length,stats[0].length);
   }

   public boolean containsParam(String param) {
      return translation.exist(param);
   }

   private boolean fillRow(String[][] stats, int i, String row) {
      if (!okRow(row)) return false;
      String[] split = row.split(this.sep);
      if (trace) log.trace("Filling " + Arrays.toString(split));
      int j = 0;
      for (String s : split) {
         stats[i][j] = s;
         j++;
      }
      return true;
   }

   private int getNumRows(String f) throws IOException {
      int i = 0;
      BufferedReader br = new BufferedReader(new FileReader(new File(f)));
      String read = br.readLine(); //do not consider header!
      while ((read = br.readLine()) != null) {
         if (okRow(read)) {
            i++;
         }
      }
      br.close();
      return i;
   }

   protected int idWithStringColumn(String header, String equalsTo) throws ParameterNotFoundException {
      List<String> stringColumn = null;
      try {
         stringColumn = rawColumn(header);
      } catch (ParameterNotFoundException e) {
         throw new ParameterNotFoundException("I could not find a " + header + " equals to " + equalsTo);
      }
      for (int i = 0; i < stringColumn.size(); i++) {
         if (stringColumn.get(i).equals(equalsTo))
            return i;
      }
      return -1;
   }

   /**
    * A row is ok to be parsed if it has the desired index and the corresponding value is ok or if it does not have the
    * desired parameter
    *
    * @param r
    * @return
    */
   private boolean okRow(String r) {
      if (!filterLines)
         return true;

      String[] split = r.split(sep);
      try {
         int ind = translation.getParamIndex("K");
         double neigh = Double.parseDouble(split[ind]);
         boolean ok = (neigh == 9.0);
         if (ok) {
            if (trace) log.trace("OK " + Arrays.toString(split));
         }
         return ok;
      } catch (ParameterNotFoundException p) {
         if (trace) log.trace("OK " + Arrays.toString(split));

         return true;
      }
   }


   public final T getParam(String param, int rowIndex) throws ParameterNotFoundException {
      final int index = translation.getParamIndex(param);
      try { return _getParam(stats[rowIndex][index]); } catch (NullPointerException n) {
         System.err.println("row " + rowIndex + " param " + param);
         n.printStackTrace();
         throw new RuntimeException(n);
      }
   }

   protected int paramId(String param) throws ParameterNotFoundException {
      return translation.getParamIndex(param);
   }

   protected abstract T _getParam(String o);

   public int numRows() {
      return stats.length;
   }

   public int numColumns() {
      return numColumns;
   }

   public List<String> allParams() {
      return translation.header();
   }

   public String[] header() {
      int size = this.translation.size();
      final String[] ret = new String[size];
      for (int i = 0; i < size; i++) {
         ret[i] = this.translation.headerEntryFor(i);
      }
      return ret;
   }

   public String[] headerExcluding(String s) {
      int id = this.translation.indexForHeaderEntry(s);
      int size = this.translation.size();
      final String[] ret = new String[size - 1];

      for (int i = 0, j = 0; i < size; i++) {
         if (i != id) {
            ret[j] = this.translation.headerEntryFor(i);
            j++;
         }
      }
      return ret;
   }

   public T getAt(int row, int column) {
      return _getParam(this.stats[row][column]);
   }

   public String getRawAt(int row, int column) {
      return (this.stats[row][column]);
   }

   public void setAt(int row, int column, String value) {
      this.stats[row][column] = value;
   }

   public void dumpToFile(String out) throws IOException {
      dumpToFileOnlyColumns(out, allRowsIndices());
   }

   private List<Integer> allRowsIndices() {
      List<Integer> list = new ArrayList<>();
      for (int i = 0; i < numRows(); i++) {
         list.add(i);
      }
      return list;
   }

   public void dumpToFileOnlyRows(String out, List<Integer> toCopyRow) throws IOException {
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      pw.print(translation.headerEntryFor(0));
      for (int i = 1; i < numColumns; i++) {
         pw.print(",");
         pw.print(translation.headerEntryFor(i));
      }
      for (int r = 0; r < numRows; r++) {
         if (!toCopyRow.contains(r))
            continue;
         pw.print("\n");
         for (int c = 0; c < numColumns; c++) {
            if (c != 0) {
               pw.print(",");
            }
            pw.print(stats[r][c]);
         }
      }
      pw.flush();
      pw.close();
   }

   public void dumpToFileOnlyColumns(String out, List<Integer> toCopyColumns) throws IOException {
      PrintWriter pw = new PrintWriter(new FileWriter(new File(out)));
      pw.print(translation.headerEntryFor(0));
      for (int i = 1; i < toCopyColumns.size(); i++) {
         pw.print(",");
         pw.print(translation.headerEntryFor(toCopyColumns.get(i)));
      }
      for (int r = 0; r < numRows; r++) {
         pw.print("\n");
         for (int c = 0; c < numColumns; c++) {
            if (toCopyColumns.contains(c)) {
               if (c != 0) {      //I assume c == 0 is always copied (it is the benchmark id)
                  pw.print(",");
               }
               pw.print(stats[r][c]);
            }
         }
      }
      pw.flush();
      pw.close();
   }

   public int idFor(String h) {
      try {
         return this.translation.getParamIndex(h);
      } catch (ParameterNotFoundException e) {
         throw new RuntimeException(e);
      }
   }

}
