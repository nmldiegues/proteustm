package evaluation.workload;

import java.util.Iterator;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public interface WorkloadSelector<W extends Workload> extends Iterator<W> {
   public int size();
}
