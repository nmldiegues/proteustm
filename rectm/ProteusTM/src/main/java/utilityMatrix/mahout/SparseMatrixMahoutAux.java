package utilityMatrix.mahout;

import gnu.trove.iterator.TIntIterator;
import org.apache.mahout.cf.taste.impl.common.FastByIDMap;
import org.apache.mahout.cf.taste.impl.model.GenericDataModel;
import org.apache.mahout.cf.taste.impl.model.GenericPreference;
import org.apache.mahout.cf.taste.impl.model.GenericUserPreferenceArray;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.model.PreferenceArray;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 08/10/14
 */
public class SparseMatrixMahoutAux extends MahoutAux {

   private SparseMatrix sparseRatingMatrix;

   public SparseMatrixMahoutAux(ProfilesHolder<UserProfile> training_, int numItems, RecommenderConfig.diffNormalization diffNormalization) {
      super(training_, numItems, diffNormalization);
   }

   @Override
   protected void setupAverageValues() {
      this.sparseRatingMatrix = trainingSetAsSparseMatrix(training_);
      setupNormalizationMetaData(sparseRatingMatrix);
   }

   @Override
   protected DataModel _fromProfilesHolderToGenericDataModel(int targetIndex) {
      return fromProfilesHolderToGenericDataModelUsingSparseMatrix(this.sparseRatingMatrix, targetIndex);
   }

   private DataModel fromProfilesHolderToGenericDataModelUsingSparseMatrix(SparseMatrix ratingMatrix, int targetItem) {
      List<GenericPreference> preferenceListForUserR;
      float rating;
      FastByIDMap<PreferenceArray> map = new FastByIDMap<>();
      GenericPreference gp;
      boolean allItems = targetItem == _ALL_ITEMS_;
      for (Integer row : ratingMatrix.unsortedRows()) {
         preferenceListForUserR = new ArrayList<>();
         //if you have to copy all items or the target row contains the target index
         if (allItems || sparseRatingMatrix.get(row, targetItem) != null) {
            for (Integer column : ratingMatrix.unsortedColumns(row)) {
               rating = (float) ratingMatrix.get(row, column).doubleValue();
               gp = new GenericPreference(row, column, rating);
               preferenceListForUserR.add(gp);
            }
         }
         GenericUserPreferenceArray arrayForUserR = new GenericUserPreferenceArray(preferenceListForUserR);
         map.put(row, arrayForUserR);
      }
      return new GenericDataModel(map);
   }

   /**
    * Setup the Maps for row-wise and column-wise average values According to the order of "differentiation", the
    * row/column avg values are taken after having removed column/row average values, so as to obtain in the end a zero
    * mean matrix by row only, column only or by both column and row
    *
    * @param matrix
    */
   private void setupNormalizationMetaData(SparseMatrix matrix) {
      Map<Integer, Double> avgUserRatings = null, avgItemRatings = null;
      //The mean for user and items has to be computed in order.
      //I.e., I compute the mean and normalize w.r.t. one, then take the mean and normalize w.r.t. the other
      if (diffNormalization.equals(RecommenderConfig.diffNormalization.USER_ITEM) || diffNormalization.equals(RecommenderConfig.diffNormalization.USER_ONLY)) {
         //Compute mean w.r.t to user and normalize
         avgUserRatings = computeAvgForUsers(matrix);
         diffNormWrtUser(matrix, avgUserRatings);
         if (diffNormalization.equals(RecommenderConfig.diffNormalization.USER_ITEM)) {
            //Compute mean w.r.t item and normalize
            avgItemRatings = computeAvgForItems(matrix);
            diffNormWrtItem(matrix, avgItemRatings);
         }
      } else if (diffNormalization.equals(RecommenderConfig.diffNormalization.ITEM_USER) || diffNormalization.equals(RecommenderConfig.diffNormalization.ITEM_ONLY)) {
         //Compute mean w.r.t. item and normalize
         avgItemRatings = computeAvgForItems(matrix);
         diffNormWrtItem(matrix, avgItemRatings);
         if (diffNormalization.equals(RecommenderConfig.diffNormalization.ITEM_USER)) {
            //Compute mean w.r.t. user and normalize
            avgUserRatings = computeAvgForUsers(matrix);
            diffNormWrtUser(matrix, avgUserRatings);
         }
      }
      this.itemAvgs = avgItemRatings;
      this.userAvgs = avgUserRatings;
   }

   /**
    * A sparse matrix
    *
    * @param training_
    * @return
    */
   private SparseMatrix trainingSetAsSparseMatrix(ProfilesHolder<UserProfile> training_) {
      //Items ranked by users are not the same, so we need to compute who has what and then average
      final SparseMatrix sparseMatrix = new SparseMatrix();
      for (int userId : training_.getUserIds()) {
         UserProfile u = training_.getProfile(userId);
         TIntIterator it = u.getItems();
         int itemId;
         while (it.hasNext()) {
            itemId = it.next();
            sparseMatrix.insertValue(userId, itemId, u.getRating(itemId)); //items are 1-based
         }
      }
      return sparseMatrix;
   }

   /**
    * Compute the average value of each column in the input SparseMatrix
    *
    * @param rankings
    * @return
    */
   private Map<Integer, Double> computeAvgForItems(SparseMatrix rankings) {
      final Map<Integer, Double> columnSumMap = new HashMap<>();
      final Map<Integer, Double> countSumMap = new HashMap<>();
      final Map<Integer, Double> avgColumnMap = new HashMap<>();
      //We go through the matrix by row to populate sum and count
      for (Integer row : rankings.unsortedRows()) {
         for (Integer column : rankings.unsortedColumns(row)) {
            double ranking = rankings.get(row, column);
            if (!columnSumMap.containsKey(column)) {
               columnSumMap.put(column, ranking);
               countSumMap.put(column, 1D);
            } else {
               double currentRanking = columnSumMap.remove(column);
               double currentCount = countSumMap.remove(column);
               columnSumMap.put(column, currentRanking + ranking);
               countSumMap.put(column, currentCount + 1D);
            }
         }
      }
      //Go through sum and count to compute average values
      for (Integer column : columnSumMap.keySet()) {
         double sum = columnSumMap.get(column);
         double count;
         if (!countSumMap.containsKey(column) || (count = countSumMap.get(column)) == 0) {
            throw new RuntimeException("WTF?");
         }
         avgColumnMap.put(column, sum / count);
      }
      return avgColumnMap;
   }

   /**
    * Removes to each element (R,C) in the matrix the value corresponding to key C in the averages Map It has
    * side-effect on the input SparseMatrix matrix
    *
    * @param matrix
    * @param averages
    */
   private void diffNormWrtItem(SparseMatrix matrix, Map<Integer, Double> averages) {
      //We don't have an immediate way to get a column, so we go through the matrix row by row
      for (Integer row : matrix.unsortedRows()) {
         //Remove to each element of row u the corresponding average value
         for (Integer column : matrix.unsortedColumns(row)) {
            double avgForItem = averages.get(column);
            double curr = matrix.get(row, column);
            curr -= avgForItem;
            matrix.updateValue(row, column, curr);
         }
      }
   }

   /**
    * Removes to each element (R,C) in the matrix the value corresponding to key R in the averages Map It has
    * side-effect on the input SparseMatrix matrix
    *
    * @param matrix
    * @param averages
    */
   private void diffNormWrtUser(SparseMatrix matrix, Map<Integer, Double> averages) {
      for (Integer row : matrix.unsortedRows()) {
         double avgForUser = averages.get(row);
         //Remove to each element of row u the corresponding average value
         for (Integer column : matrix.unsortedColumns(row)) {
            double curr = matrix.get(row, column);
            curr -= avgForUser;
            matrix.updateValue(row, column, curr);
         }
      }
   }

   /**
    * Compute the average value of each row in the input SparseMatrix
    *
    * @param rankings
    * @return
    */
   private Map<Integer, Double> computeAvgForUsers(SparseMatrix rankings) {
      Map<Integer, Double> map = new HashMap<>();
      //For each user, scan horizontally by item and compute the avg
      for (Integer row : rankings.unsortedRows()) {
         double sum = 0;
         double count = 0;
         double avg = 0;
         for (Integer column : rankings.unsortedColumns(row)) {
            sum += rankings.get(row, column);
            count++;
         }
         if (count > 0) {
            avg = sum / count;
         } else {
            throw new IllegalArgumentException("Row " + row + " should not be empty");
         }
         map.put(row, avg);
      }
      return map;
   }

   private class SparseMatrix {
      private Map<Integer, SparseArray> matrix;

      private SparseMatrix() {
         this.matrix = new HashMap<Integer, SparseArray>();
      }

      public Double get(Integer row, Integer column) {
         if (column == 0) {
            throw new IllegalArgumentException("Items are supposed to be 1-bsed");
         }
         if (!this.matrix.containsKey(row)) {
            throw new IllegalArgumentException("Row " + row + " is not here");
         }
         final SparseArray sparseArray = matrix.get(row);
         if (!sparseArray.containsKey(column)) {
            throw new IllegalArgumentException("Row " + row + " does not contain column " + column);
         }
         return sparseArray.get(column);
      }

      public Set<Integer> unsortedRows() {
         return matrix.keySet();
      }

      /**
       * Get the columns for the specified row
       *
       * @param row
       * @return
       */
      public Set<Integer> unsortedColumns(Integer row) {
         if (this.matrix.containsKey(row)) {
            return this.matrix.get(row).unsortedIndices();
         }
         throw new RuntimeException("Row " + row + " is not present");
      }

      /**
       * Put a value at a (row, column)
       *
       * @param row
       * @param column
       * @param value
       * @param assertBlank true if you want to throw an Exception if a previous value has already been specified
       * @return
       */
      private Double putValue(Integer row, Integer column, Double value, boolean assertBlank) {
         if (column == 0) {
            throw new IllegalArgumentException("Items are supposed to be 1-based");
         }
         if (!matrix.containsKey(row)) {
            matrix.put(row, new SparseArray());
         }
         return matrix.get(row).putValue(column, value, assertBlank);
      }

      /**
       * Insert a value at a (row, column) that is empty
       *
       * @param row
       * @param column
       * @param value
       * @return
       */
      public Double insertValue(Integer row, Integer column, Double value) {
         return this.putValue(row, column, value, true);
      }

      /**
       * Update the value at a (row, column) that has already been initialized
       *
       * @param row
       * @param column
       * @param value
       * @return
       */
      public Double updateValue(Integer row, Integer column, Double value) {
         return this.putValue(row, column, value, false);
      }
   }

   private class SparseArray {
      private Map<Integer, Double> array;

      private SparseArray() {
         this.array = new HashMap<>();
      }

      public Double get(int index) {
         if (this.array.containsKey(index)) {
            return this.array.get(index);
         }
         throw new IllegalArgumentException("Index " + index + " is not here");
      }

      public boolean containsKey(Integer key) {
         return this.array.containsKey(key);
      }

      public Set<Integer> unsortedIndices() {
         return array.keySet();
      }

      private Double putValue(Integer index, Double value, boolean assertBlank) {
         if (array.containsKey(index) && assertBlank) {
            throw new RuntimeException("Element " + index + " is already present");
         }
         return this.array.put(index, value);
      }

      public Double insertValue(Integer index, Double value) {
         return this.putValue(index, value, true);
      }

      public Double updateValue(Integer index, Double value) {
         return this.putValue(index, value, false);
      }
   }
}
