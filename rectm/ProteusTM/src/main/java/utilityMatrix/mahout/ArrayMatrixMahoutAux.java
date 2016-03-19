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

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 08/10/14
 */
public class ArrayMatrixMahoutAux extends MahoutAux {

   private Double[][] ratingMatrix = null;

   public ArrayMatrixMahoutAux(ProfilesHolder<UserProfile> training_, int numItems, RecommenderConfig.diffNormalization diffNormalization) {
      super(training_, numItems, diffNormalization);
   }

   @Override
   protected void setupAverageValues() {
      this.ratingMatrix = trainingSetAsMatrix(training_, numItems);
      setupNormalizationMetaData(this.ratingMatrix);
   }

   private void printMatrixWithHoles(double[][] matrix) {
      final int columns = matrix[0].length;
      for (double[] aMatrix : matrix) {
         StringBuilder sb = new StringBuilder();
         for (int c = 0; c < columns; c++) {
            if (c > 0) {
               sb.append(",");
            }
            sb.append(aMatrix[c]);
         }
         System.out.println(sb.toString());
      }
   }


   /**
    * Express the trainingset as a Double matrix, having null where we do not have a preference
    *
    * @param training_
    * @param numItems
    * @return
    */
   private Double[][] trainingSetAsMatrix(ProfilesHolder<UserProfile> training_, int numItems) {
      int numUsers = training_.getNbUsers();
      //Items ranked by users are not the same, so we need to compute who has what and then average
      Double[][] rankings = new Double[numUsers][numItems];
      for (int userId : training_.getUserIds()) {
         UserProfile u = training_.getProfile(userId);
         TIntIterator it = u.getItems();
         int itemId;
         while (it.hasNext()) {
            itemId = it.next();
            rankings[userId][itemId - 1] = u.getRating(itemId); //items are 1-based
         }
      }
      return rankings;
   }


   /**
    * Remove to each element of row u the corresponding average value
    *
    * @param matrix
    * @param averages
    */
   private void diffNormWrtUser(Double[][] matrix, Map<Integer, Double> averages) {
      int u = 0;
      for (Double[] D : matrix) {
         if (!averages.containsKey(u)) {
            throw new IllegalArgumentException("Average not available for user " + u);
         }
         double avgForUser = averages.get(u);
         //Remove to each element of row u the corresponding average value
         for (int i = 0; i < D.length; i++) {
            if (D[i] != null) {
               D[i] -= avgForUser;
            }
         }
         u++;
      }
   }

   /**
    * Remove to each element of column u the corresponding average value
    *
    * @param matrix
    * @param averages
    */
   private void diffNormWrtItem(Double[][] matrix, Map<Integer, Double> averages) {
      for (int i = 0; i < matrix[0].length; i++) {
         int itemIndex = i + 1;
         if (!averages.containsKey(itemIndex)) {
            throw new IllegalArgumentException("Average not available for item " + itemIndex);
         }
         double avgPerItem = averages.get(itemIndex);    //Items are 1-based!
         for (Double[] D : matrix) {
            if (D[i] != null) {
               D[i] -= avgPerItem;
            }
         }
      }
   }

   private void setupNormalizationMetaData(Double[][] matrix) {
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


   @Override
   protected DataModel _fromProfilesHolderToGenericDataModel(int targetItem) {
      return fromProfilesHolderToGenericDataModelUsingMatrix(this.ratingMatrix, targetItem);
   }

   private DataModel fromProfilesHolderToGenericDataModelUsingMatrix(Double[][] ratingMatrix, int targetItem) {
      final int numRows = ratingMatrix.length;
      final int numColumns = ratingMatrix[0].length;
      double[][] matrix = (trace ? new double[numRows][numColumns] : null);
      List<GenericPreference> preferenceListForUserR;
      float rating;
      FastByIDMap<PreferenceArray> map = new FastByIDMap<>();
      GenericPreference gp;
      final boolean allItems = targetItem == _ALL_ITEMS_;
      int r = 0;
      for (Double[] user : ratingMatrix) {
         preferenceListForUserR = new ArrayList<>();
         for (int i = 0; i < user.length; i++) {
            //Copy if the user is non-null and (either we copy all items or the target item (1-based) is in the user)
            if (user[i] != null && (allItems || user[targetItem - 1] != null)) {
               rating = (float) user[i].doubleValue();
               gp = new GenericPreference(r, i + 1, rating);  //Items are 1-based!
               preferenceListForUserR.add(gp);
               if (trace) {matrix[r][i] = rating;}
            }
         }
         GenericUserPreferenceArray arrayForUserR = new GenericUserPreferenceArray(preferenceListForUserR);
         map.put(r, arrayForUserR);
         r++;
      }
      if (trace) {printMatrixWithHoles(matrix);}
      return new GenericDataModel(map);
   }


   private Map<Integer, Double> computeAvgForItems(Double[][] rankings) {
      Map<Integer, Double> map = new HashMap<>();
      int numItems = rankings[0].length;
      //For each item, scan vertically by user and compute the avg
      for (int i = 0; i < numItems; i++) {
         double sum = 0;
         double count = 0;
         double avg = 0;
         for (Double[] ranking : rankings) {
            if (ranking[i] != null) {
               sum += ranking[i];
               count++;
            }
         }
         if (count > 0) {
            avg = sum / count;
         }
         map.put(i + 1, avg); //Items are 1 based!
      }
      return map;
   }


   private Map<Integer, Double> computeAvgForUsers(Double[][] rankings) {
      Map<Integer, Double> map = new HashMap<>();
      int numItems = rankings[0].length;
      //For each user, scan horizontally by item and compute the avg
      int r = 0;
      for (Double[] ranking : rankings) {
         double sum = 0;
         double count = 0;
         double avg = 0;
         for (int i = 0; i < numItems; i++) {
            if (ranking[i] != null) {
               sum += ranking[i];
               count++;
            }
         }
         if (count > 0) {
            avg = sum / count;
         }
         map.put(r, avg);
         r++;
      }
      return map;
   }

   public void printMatrix() {
      if (this.ratingMatrix != null) {
         for (Double[] d : this.ratingMatrix) {
            for (Double aD : d) {
               System.out.print(aD + ", ");
            }
            System.out.print("\n");
         }
      } else if (this.training_ != null) {
         for (int i : this.training_.getUserIds()) {
            System.out.println(i + ": " + training_.getProfile(i));
         }
      } else throw new RuntimeException("No training!");
   }

   private static Double[][] copyDoubleMatrix(Double[][] input) {
      final int nR = input.length;
      final int nC = input[0].length;
      final Double[][] output = new Double[nR][nR];
      for (int r = 0; r < nR; ++r) {
         for (int c = 0; c < nC; ++c) {
            if (input[r][c] != null) {
               output[r][c] = input[r][c];
            }
         }
      }
      return output;
   }

}
