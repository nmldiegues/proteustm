package evaluation.workload;

import evaluation.configuration.Configuration;
import utilityMatrix.recoEval.profiles.UserProfile;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public interface Workload<W extends Workload<W>> {
   public int getId();

   public double kpiFor(Configuration c);

   public void add(Configuration c, double rating);

   public void remove(Configuration c);

   public UserProfile getProfile();

   public W emptyWorkload();

   public int size();
}
