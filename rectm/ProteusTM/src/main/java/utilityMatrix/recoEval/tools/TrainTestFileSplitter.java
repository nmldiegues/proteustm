package utilityMatrix.recoEval.tools;

import au.com.bytecode.opencsv.CSVReader;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 25/07/14
 */
public class TrainTestFileSplitter {
   private final File originalFile;
   private final static boolean _TRAIN_ = true;
   private final long seed;
   private final static Log log = LogFactory.getLog(TrainTestFileSplitter.class);


   public TrainTestFileSplitter(File f, long seed) {
      originalFile = f;
      this.seed = seed;
   }

   public File trainSplit(int numRowsToKeep, String out) {
      final int totalRows = MathTools.countRowsInFile(originalFile, false);
      log.trace("Total number of rows in full set (excluding header)" + totalRows);
      return split(numRowsToKeep, totalRows, this.seed, new File(out), _TRAIN_);
   }

   public File testSplit(int numRowsToKeep, String out) {
      final int totalRows = MathTools.countRowsInFile(originalFile, false);
      return split(numRowsToKeep, totalRows, this.seed, new File(out), !_TRAIN_);
   }

   private File split(int numRowsToKeep, int totalRows, long randomSeed, File out, boolean isTrain) {
      List<Integer> selectedRowsForTrain = MathTools.drawIndicesUniformly(numRowsToKeep, totalRows, randomSeed);
      log.trace("" + selectedRowsForTrain.size() + " selected rows " + selectedRowsForTrain);
      return split(selectedRowsForTrain, out, isTrain);

   }

   private File split(List<Integer> rowsToKeep, File out, boolean isTrain) {
      try {
         CSVReader csvReader = new CSVReader(new FileReader(this.originalFile));
         //CSVWriter csvWriter = new CSVWriter(new FileWriter(out));
         PrintWriter csvWriter = new PrintWriter(new FileWriter(out));
         String[] read;
         int index = 0;
         //Write the header
         // csvWriter.writeNext(csvReader.readNext());
         csvWriter.println(StringArrayToCSVString(csvReader.readNext()));
         while ((read = csvReader.readNext()) != null) {
            if (keepRow(index, rowsToKeep, isTrain)) {
               //csvWriter.writeNext(StringArrayToCSVString(read));
               csvWriter.println(StringArrayToCSVString(read));
            }
            index++;
         }
         csvWriter.close();
         csvReader.close();
      } catch (IOException e) {
         e.printStackTrace();  // TODO: Customise this generated block
      }
      return out;
   }

   private boolean keepRow(int row, List<Integer> list, boolean isTrain) {
      String t = isTrain ? "TRAIN" : "TEST";
      final boolean contain = list.contains(row);
      log.trace(t + " am I keeping row " + row + "? ");
      final boolean willKeep;
      if (isTrain) {
         willKeep = contain;
      } else {
         willKeep = !contain;
      }
      final String y = willKeep ? "YES" : "NO";
      log.trace(y);
      return willKeep;
   }


   private String StringArrayToCSVString(String[] array) {
      StringBuilder sb = new StringBuilder(array[0]);
      for (int i = 1; i < array.length; i++) {
         sb.append(",");
         sb.append(array[i]);
      }
      return sb.toString();
   }

}
