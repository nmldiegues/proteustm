package utilityMatrix.recoEval.datasetLoader;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import utilityMatrix.recoEval.tools.ExtendedParameters;
import utilityMatrix.recoEval.tools.MathTools;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 25/07/14
 */

/**
 * This works this way. It draws N rows and these N rows will be complete for train and absent for test Then, it will
 * put, for each of the remaining rows, C columns extracted randomly in the train. The test will be its complement
 */
public class FixedEntriesCSVLoader extends CSVLoaderSplit {
   //Number of complete rows (users) for the training set
   private final int rowsInTrain;
   //Total number of row in training+test set
   private final int totalNumberOfRow;
   //For the non-full rows, number of column to fill. Such rows correspond to the test set for which only some values are known
   private final int cellsPerRowToFillInTrainingMode;
   //Total number of columns (rankings) per row
   private final int totalCellsPerRow;
   private final static Log log = LogFactory.getLog(FixedEntriesCSVLoader.class);
   //Columns to pick for the current row
   protected List<Integer> currentRowPickedColumnsForTraining;
   //Rows to be filled up completely for the training
   protected List<Integer> pickedRowsForTraining;
   //If !=null, these are the columns that are common to all elements in the test set (i.e., the non-full rows).
   //The others are drawn uniformly
   private final int[] fixedIndices;

   private final int trainEntryProb;

   public static final boolean TRAIN = true;
   public static final boolean TEST = false;


   private List<Integer> toList(int[] i) {
      ArrayList<Integer> toRet = new ArrayList<>();
      for (int ii : i)
         toRet.add(ii);
      return toRet;
   }

   public List<Integer> getPickedRowsForTraining() {
      return pickedRowsForTraining;
   }

   //TODO: I could select/draw more quickly the elements by keeping in the "currentRowPickedColumnsForTraining"
   //TODO: the currentRowPickedColumnsForTraining I have to retain (regardless of the train/test)

   @Override
   protected void _newLine(int line) {
      //log.trace("Columns to fill " + toFill);
      final int fixed;
      if (fixedIndices == null) {
         fixed = 0;
      } else {
         fixed = fixedIndices.length;
      }
      final int lastValidIndex = totalCellsPerRow - 1;
      //Draw UNIFORMLY
      //Columns are from 0 to totaCell-1, but the 0-th one is the label so it gets discarded
      if (trace) log.trace("line " + line + " " + currentRowPickedColumnsForTraining);
      //If you have some fixed columns...
      if (fixed >= 1) {
         //Pick uniformly being sure you do not pick the fixed indices, for now
         currentRowPickedColumnsForTraining = MathTools.drawUniformlyExcluding(cellsPerRowToFillInTrainingMode - fixed, 1, lastValidIndex, this.random, toList(fixedIndices));
         //System.out.println("Drawn " + currentRowPickedColumnsForTraining.toString());
         //Now addIfAbsent the fixed elements
         assert fixedIndices != null;
         for (int i : fixedIndices) {
            currentRowPickedColumnsForTraining.add(i);
         }
      } else {
         currentRowPickedColumnsForTraining = MathTools.drawIndicesUniformly(cellsPerRowToFillInTrainingMode, 1, lastValidIndex, this.random);
      }
      if (!isTraining && trace) {
         log.trace("Row " + line + " drawn " + currentRowPickedColumnsForTraining.size() + " ==>" + currentRowPickedColumnsForTraining.toString());
      }
   }

   private boolean contains(int[] array, int i) {
      for (int j : array) {
         if (i == j) {
            return true;
         }
      }
      return false;
   }

   /**
    * NB: items are 1-based
    *
    * @param numTotalCellsPerRow
    * @return
    */
   private int[] allFixedIndices(int numTotalCellsPerRow) {
      int[] array = new int[getNumItemsPerUser()];
      for (int i = 0; i < array.length - 1; i++) {
         array[i] = i + 1;
      }
      return array;
   }

   private int[] extractFixedIndices(int cellsPerRowToFillInTrainingMode, String fixedIndices) {
      int[] toRet;
      //NB: at setting time, if the input String is empty, the instance variable is set to null
      if (fixedIndices != null) {
         if (fixedIndices.equals("ALL")) {  //Never used ;)
            toRet = allFixedIndices(totalCellsPerRow);
         } else {
            toRet = ExtendedParameters.getFixedIndexesInTrainAsIntArray(fixedIndices);
         }
         //System.out.println("FixedCSV "+ Arrays.toString(fixedIndices));
         if (toRet.length > cellsPerRowToFillInTrainingMode) {
            throw new IllegalArgumentException("You can't have more fixed indices (" + toRet.length + ") than total indices (" + cellsPerRowToFillInTrainingMode + ")");
         }
      } else {
         toRet = null;
      }
      return toRet;
   }

   public FixedEntriesCSVLoader(String file, boolean training, long seed, int rowsInTrain, int cellsPerRowToFillInTrainingMode, String fixedIndices) {
      super(file, training, seed);
      //This includes also the first, so if we have 50 cells --0 to 49-- we have the 49-th to be the last valid index
      this.totalCellsPerRow = MathTools.columnPerRow(this.srcFile, SEPARATOR, true);
      this.fixedIndices = extractFixedIndices(cellsPerRowToFillInTrainingMode, fixedIndices);
      this.cellsPerRowToFillInTrainingMode = Math.min(cellsPerRowToFillInTrainingMode, totalCellsPerRow - 1);
      this.rowsInTrain = rowsInTrain;
      this.totalNumberOfRow = MathTools.countRowsInFile(this.srcFile, false);
      this.pickedRowsForTraining = fullRowsToTrain();
      if (trace) log.trace(this);
      trainEntryProb = 100;
   }

   public FixedEntriesCSVLoader(String file, boolean training, long seed, int rowsInTrain, int cellsPerRowToFillInTrainingMode, String fixedIndices, int trainEntryProb) {
      super(file, training, seed);
      //This includes also the first, so if we have 50 cells --0 to 49-- we have the 49-th to be the last valid index
      this.totalCellsPerRow = MathTools.columnPerRow(this.srcFile, SEPARATOR, true);
      this.fixedIndices = extractFixedIndices(cellsPerRowToFillInTrainingMode, fixedIndices);
      this.cellsPerRowToFillInTrainingMode = Math.min(cellsPerRowToFillInTrainingMode, totalCellsPerRow - 1);
      this.rowsInTrain = rowsInTrain;
      this.totalNumberOfRow = MathTools.countRowsInFile(this.srcFile, false);
      this.pickedRowsForTraining = fullRowsToTrain();
      this.trainEntryProb = trainEntryProb;
      if (trace) log.trace(this);
   }


   /**
    * Copy the selected cell if needed
    *
    * @param rowIndex
    * @param columnIndex
    * @return
    */
   protected boolean copyCell(int rowIndex, int columnIndex) {
      //just to keep legacy code...
      if (trainEntryProb == 100) {
         //If I am training, either I copy a whole row, or a selected column
         if (isTraining) {
            return pickedRowsForTraining.contains(rowIndex) || currentRowPickedColumnsForTraining.contains(columnIndex);
         }
         //If I am testing I have to filter selected rows in train and the picked columns
         //NB: this is the negation of previous predicate !(A || B) = !A !B
         else {
            return !pickedRowsForTraining.contains(rowIndex) && !currentRowPickedColumnsForTraining.contains(columnIndex);
         }
      } else {
         //If the column has to be in each row, then pick it only if we are in training
         if (currentRowPickedColumnsForTraining.contains(columnIndex))
            return isTraining;
         //If the row has to be in training, if the prob is 100, pick it, with a gieven prob, if we are in training
         if (pickedRowsForTraining.contains(rowIndex)) {
            if (trainEntryProb == 100) {
               return isTraining;
            } else {
               //The modulo is to avoid that when the r*c product is the same, we always get the same result (the +1 to avoid 0's)
               final long seed = rowIndex * columnIndex * this.fixedSeed * ((rowIndex % columnIndex) + 1);
               final Random r = new Random(seed);
               final int ran = 1 + r.nextInt(99);
               if (isTraining) {
                  return ran <= trainEntryProb;
               } else {
                  return ran > trainEntryProb;
               }
            }
         }
         //If I am in training and I am not picking the column, nor I am picking the row, I only have to add this if I am in testing mode
         return !isTraining;
      }
   }


   protected List<Integer> fullRowsToTrain() {
      //The rows are 0-indexed, as the header is excluded
      return MathTools.drawIndicesUniformly(this.rowsInTrain, 0, this.totalNumberOfRow - 1, this.random);
   }

   @Override
   public String toString() {
      return "FixedEntriesCSVLoader{" +
            "rowsInTrain=" + rowsInTrain +
            ", totalNumberOfRow=" + totalNumberOfRow +
            ", cellsPerRowToFillInTrainingMode=" + cellsPerRowToFillInTrainingMode +
            ", totalCellsPerRow=" + totalCellsPerRow +
            ", currentRowPickedColumnsForTraining=" + currentRowPickedColumnsForTraining +
            ", pickedRowsForTraining=" + pickedRowsForTraining +
            '}';
   }

   public int getNumItemsPerUser() {
      return totalCellsPerRow - 1;
   }
}
