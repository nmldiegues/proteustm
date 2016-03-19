package utilityMatrix.mahout;

import gnu.trove.iterator.TIntIterator;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.mahout.cf.taste.impl.common.FastByIDMap;
import org.apache.mahout.cf.taste.impl.common.LongPrimitiveIterator;
import org.apache.mahout.cf.taste.impl.model.GenericDataModel;
import org.apache.mahout.cf.taste.impl.model.GenericPreference;
import org.apache.mahout.cf.taste.impl.model.GenericUserPreferenceArray;
import org.apache.mahout.cf.taste.impl.model.file.FileDataModel;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.model.PreferenceArray;
import utilityMatrix.RecommenderConfig;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;

import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 30/08/14
 */
public abstract class MahoutAux {

   protected final static Log log = LogFactory.getLog(MahoutAux.class);
   protected final static boolean trace = log.isTraceEnabled();
   protected RecommenderConfig.diffNormalization diffNormalization = null;
   protected final ProfilesHolder<UserProfile> training_;
   protected final int numItems;
   protected Map<Integer, Double> userAvgs = null;
   protected Map<Integer, Double> itemAvgs = null;

   protected final static int _ALL_ITEMS_ = -1;


   public MahoutAux(ProfilesHolder<UserProfile> training_, int numItems, RecommenderConfig.diffNormalization diffNormalization) {
      this.training_ = training_;
      this.numItems = numItems;
      this.diffNormalization = diffNormalization;
      if (diffNormalization != null && diffNormalization != RecommenderConfig.diffNormalization.NONE) {
         setupAverageValues();
      }
   }

   protected abstract void setupAverageValues();

   protected abstract DataModel _fromProfilesHolderToGenericDataModel(int targetItem);

   /**
    * Creates a FileDataModel starting from a ProfilesHolder. It uses an auxiliary file (outfile). Therefore, this
    * method is not thread-safe
    *
    * @param trainingProfiles
    * @param numColumns
    * @param outFile
    * @return a FileDataModel
    */
   @Deprecated
   public DataModel fromProfilesHolderToFileDataModel(ProfilesHolder<UserProfile> trainingProfiles, int numColumns, String outFile) {

      if (trace) {
         log.trace("Num rows in training " + trainingProfiles.getNbUsers());
         log.trace("Num columns " + numColumns);
      }

      try {
         final File f = new File(outFile);
         final PrintWriter pw = new PrintWriter(new FileWriter(f));
         String fileDataModelLine;
         for (int i : trainingProfiles.getUserIds()) {
            UserProfile up = trainingProfiles.getProfile(i);
            TIntIterator it = up.getItems();
            while (it.hasNext()) {
               int j = it.next();
               fileDataModelLine = i + "," + j + "," + trainingProfiles.getProfile(i).getRating(j);
               if (trace) {
                  log.trace(fileDataModelLine);
               }
               pw.println(fileDataModelLine);
            }
         }
         pw.flush();
         pw.close();
         return new FileDataModel(f);
      } catch (Exception e) {
         e.printStackTrace();
         throw new RuntimeException(e);
      }
   }

   private DataModel fromProfilesHolderToGenericDataModelUsingProfilesHolder(ProfilesHolder<UserProfile> trainingProfiles, int numColumns, int targetItem) {
      UserProfile userProfileI;
      List<GenericPreference> preferenceListForUserI;
      TIntIterator itForUserI;
      float rating;
      FastByIDMap<PreferenceArray> map = new FastByIDMap<>();
      GenericPreference gp;
      int r = 0;
      final boolean allItems = targetItem == _ALL_ITEMS_;
      //System.out.println(Arrays.toString(trainingProfiles.getUserIds()));
      for (int userId : trainingProfiles.getUserIds()) {
         userProfileI = trainingProfiles.getProfile(userId);
         itForUserI = userProfileI.getItems();
         preferenceListForUserI = new ArrayList<>();
         while (itForUserI.hasNext()) {
            if (!allItems && !userProfileI.contains(targetItem)) {
               continue;
            }
            int itemId = itForUserI.next();
            //sanity check
            if (!userProfileI.contains(itemId)) {
               throw new RuntimeException("User " + userId + " does not contain rating for " + itemId);
            }
            rating = (float) userProfileI.getRating(itemId);
            gp = new GenericPreference(userId, itemId, rating);
            preferenceListForUserI.add(gp);
         }
         if (!preferenceListForUserI.isEmpty()) {
            GenericUserPreferenceArray arrayForUserI = new GenericUserPreferenceArray(preferenceListForUserI);
            map.put(userId, arrayForUserI);
         }
         ++r;
      }

      /* Just for debugging purposes*/
      if (false) {
         for (LongPrimitiveIterator longPrimitiveIterator = map.keySetIterator(); longPrimitiveIterator.hasNext(); ) {
            long l = longPrimitiveIterator.nextLong();
            System.out.println(map.get(l));
         }
      }

      if (false) {
         final int numRows = trainingProfiles.getUserIds().length;
         Double[][] matrix = new Double[numRows][numColumns];
         for (r = 0; r < numRows; r++) {
            matrix[r] = new Double[numColumns];
            UserProfile u = trainingProfiles.getProfile(r);
            for (int c = 0; c < numColumns; c++) {
               int itemIndex = c + 1;
               Double v = (u == null || !u.contains(itemIndex)) ? null : u.getRating(itemIndex);
               matrix[r][c] = v;
            }
            System.out.println(Arrays.toString(matrix[r]));
         }
      }
      return new GenericDataModel(map);
   }

   public DataModel fromProfilesHolderToGenericDataModel() {
      if (this.diffNormalization == null || diffNormalization.equals(RecommenderConfig.diffNormalization.NONE)) {
         return fromProfilesHolderToGenericDataModelUsingProfilesHolder(this.training_, this.numItems, _ALL_ITEMS_);
      }
      return _fromProfilesHolderToGenericDataModel(_ALL_ITEMS_);
   }

   public DataModel fromProfilesHoldersOnlyWithTargetItemToGenericDataModel(int targetItem) {
      if (this.diffNormalization == null || diffNormalization.equals(RecommenderConfig.diffNormalization.NONE)) {
         return fromProfilesHolderToGenericDataModelUsingProfilesHolder(this.training_, this.numItems, targetItem);
      }
      return _fromProfilesHolderToGenericDataModel(targetItem);
   }

   public double avgForUser(int user) {
      return this.userAvgs.get(user);
   }

   public double avgForItem(int item) {
      if (item == 0) {
         throw new IllegalArgumentException("Items are supposed to be 1-based!");
      }
      return this.itemAvgs.get(item);
   }

   @Override
   public String toString() {
      return "MahoutAux{" +
            "diffNormalization=" + diffNormalization +
            ", training_=" + training_ +
            ", numItems=" + numItems +
            ", userAvgs=" + userAvgs +
            ", itemAvgs=" + itemAvgs +
            '}';
   }
}
