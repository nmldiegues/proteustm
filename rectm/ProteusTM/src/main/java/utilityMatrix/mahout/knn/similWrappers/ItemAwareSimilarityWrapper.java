package utilityMatrix.mahout.knn.similWrappers;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.mahout.cf.taste.common.Refreshable;
import org.apache.mahout.cf.taste.common.TasteException;
import org.apache.mahout.cf.taste.model.DataModel;
import org.apache.mahout.cf.taste.model.PreferenceArray;
import org.apache.mahout.cf.taste.similarity.PreferenceInferrer;
import org.apache.mahout.cf.taste.similarity.UserSimilarity;

import java.util.Collection;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 10/11/14
 */
public class ItemAwareSimilarityWrapper implements UserSimilarity {

   private UserSimilarity actualUserSimilarity;
   private int targetItem;
   private DataModel fullDataModel;
   private final static Log log = LogFactory.getLog(ItemAwareSimilarityWrapper.class);
   private final boolean trace = log.isTraceEnabled();


   public ItemAwareSimilarityWrapper(UserSimilarity actualUserSimilarity, int targetItem, DataModel fullDataModel) {
      this.actualUserSimilarity = actualUserSimilarity;
      this.targetItem = targetItem;
      this.fullDataModel = fullDataModel;
   }

   @Override
   public double userSimilarity(long userID1, long userID2) throws TasteException {
      if (trace) {
         log.trace("Comparing for target " + targetItem + "\n" + this.fullDataModel.getPreferencesFromUser(userID1) + "\n" + this.fullDataModel.getPreferencesFromUser(userID2));
      }
      if (!containTargetItem(userID1, userID2, this.targetItem, this.fullDataModel)) {
         if (trace) {
            log.trace("-1");
         }
         return -1;
      }
      return actualUserSimilarity.userSimilarity(userID1, userID2);
   }


   @Override
   public void setPreferenceInferrer(PreferenceInferrer inferrer) {
      this.actualUserSimilarity.setPreferenceInferrer(inferrer);
   }

   @Override
   public void refresh(Collection<Refreshable> alreadyRefreshed) {
      this.actualUserSimilarity.refresh(alreadyRefreshed);
   }

   /**
    * Return true if both users contains the target item. Note that I do not know how mahout invokes the similarity
    * methods (i.e., which users is ID1 and which is UD2) so I just enforce that at least one has it: upper methods
    * should enforce that if the user already has the item, it does not speculate about it. This is true in RecTM (we
    * never query for an item we alreay have in the training set), and this is also enforced in Mahout's code (if the
    * target is already present as a "Preference", then the actual rating is returned)
    *
    * @param userID1
    * @param userID2
    * @param targetItem
    * @return
    */
   private boolean containTargetItem(long userID1, long userID2, int targetItem, DataModel dataModel) throws TasteException {
      final PreferenceArray array1 = dataModel.getPreferencesFromUser(userID1);
      final PreferenceArray array2 = dataModel.getPreferencesFromUser(userID2);
      return array1.hasPrefWithItemID(targetItem) || array2.hasPrefWithItemID(targetItem);
   }

}
