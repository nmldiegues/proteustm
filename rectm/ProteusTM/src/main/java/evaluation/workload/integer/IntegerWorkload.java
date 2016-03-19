package evaluation.workload.integer;

import evaluation.configuration.Configuration;
import evaluation.workload.Workload;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.UserProfile;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 * <p/>
 * A workload that is characterized by its offset in the training/test set NB: the workload indices have offset 1
 */
public class IntegerWorkload implements Workload<IntegerWorkload> {

   private int id;
   private UserProfile workloadProfile;

   public UserProfile getProfile() {
      return workloadProfile;
   }

   public IntegerWorkload(int workloadIndex, UserProfile workloadProfile) {
      this.id = workloadIndex;
      this.workloadProfile = workloadProfile;
   }

   public double kpiFor(Configuration c) {
      final int cid = c.getId();
      if (workloadProfile.contains(cid)) {
         return workloadProfile.getRating(cid);
      }
      throw new IllegalArgumentException("Workload " + this + " has not kpi for config " + c);
   }

   @Override
   public void add(Configuration c, double rating) {
      final int cid = c.getId();
      final boolean added = workloadProfile.add(cid, rating);
      if (!added) {
         throw new IllegalArgumentException("Configuration " + c + " already present in " + workloadProfile);
      }
   }

   @Override
   public void remove(Configuration c) {
      final int cid = c.getId();
      final boolean removed = workloadProfile.remove(cid);
      if (!removed) {
         throw new IllegalArgumentException("Configuration " + c + " not present in " + workloadProfile);
      }
   }

   public int getId() {
      return id;
   }

   @Override
   public IntegerWorkload emptyWorkload() {
      return new IntegerWorkload(this.id, new BasicUserProfile());
   }

   @Override
   public int size() {
      return this.workloadProfile.getNbItems();
   }

   @Override
   public String toString() {
      return "IntegerWorkload{" +
            "id=" + id +
            ", workloadProfile=" + workloadProfile +
            '}';
   }
}
