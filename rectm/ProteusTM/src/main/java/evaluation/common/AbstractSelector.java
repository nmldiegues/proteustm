package evaluation.common;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public abstract class AbstractSelector<W> implements Iterator<W> {

   //Null values are not allowed in the lists
   protected List<W> bootstrapList;
   protected List<W> steadyStateList;
   protected List<W> currentList;

   protected final static Log log = LogFactory.getLog(AbstractSelector.class);
   protected final static boolean trace = log.isTraceEnabled();
   protected final static boolean debug = log.isDebugEnabled();
   protected final static boolean info = log.isInfoEnabled();

   protected int numNexted = 0;

   protected AbstractSelector(List<W> steadyStateList) {
      this.steadyStateList = steadyStateList;
      this.bootstrapList = new ArrayList<>();
      currentList = steadyStateList;
      if (currentList == null || currentList.isEmpty()) {
         throw new IllegalArgumentException("Invalid parameter");
      }
      if (trace) {
         log.trace("Steady-state elements will be " + steadyStateList);
      }
   }

   protected AbstractSelector(List<W> bootstrapList, List<W> steadyStateList) {
      this.steadyStateList = steadyStateList;
      this.bootstrapList = bootstrapList;
      if (bootstrapList.isEmpty()) {
         currentList = steadyStateList;
      } else {
         currentList = bootstrapList;
      }
      if (currentList == null || currentList.isEmpty()) {
         throw new IllegalArgumentException("Invalid parameters");
      }
      if (trace) {
         log.trace("Bootstrap elements will be " + bootstrapList);
         log.trace("Steady-state elements will be " + steadyStateList);
      }
   }

   @Override
   public void remove() {
      throw new UnsupportedOperationException("Remove is not supported");
   }


   @Override
   public boolean hasNext() {
      return size() > 0 && !terminateExploration();
   }

   //TODO: this is wrong: When I ask you for the next, you may not be able to pre-compute the next-next, as it may depend
   //TODO: on the feedback I get from the current next.
   //So we just say "hasNext == true" if the lists are not empty
   //Next is computed on the fly
   @Override
   public W next() {
      numNexted++;
      //Serve from the bootstrapList
      if (currentList == bootstrapList) {
         W toReturn = currentList.get(0);
         currentList.remove(toReturn);
         if (trace) {
            log.trace("Serving from bootstrapList " + toReturn);
         }
         //If the bootstrap list is now empty, transition to steady-state
         if (bootstrapList.isEmpty()) {
            currentList = steadyStateList;
            if (trace) {
               log.trace("Transitioning to steady-state");
            }
         }
         return toReturn;
      } else {
         W toReturn = _next();
         if (trace) {
            log.trace("Serving from steady-state " + toReturn);
         }
         currentList.remove(toReturn);
         return toReturn;
      }
   }

   public int size() {
      int bS, sS;
      bS = bootstrapList == null ? 0 : bootstrapList.size();
      sS = steadyStateList == null ? 0 : steadyStateList.size();
      return bS + sS;
   }


   protected boolean emptyList() {
      //The current is updated at next() time, so we just need to check whether the current is empty
      return currentList.isEmpty();
   }

   protected abstract W _next();

   protected abstract boolean terminateExploration();


}
