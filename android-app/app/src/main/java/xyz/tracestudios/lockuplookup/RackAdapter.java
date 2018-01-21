package xyz.tracestudios.lockuplookup;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import java.util.ArrayList;

public class RackAdapter extends ArrayAdapter<Rack> {

    ArrayList<Rack> racks = new ArrayList<>();
    // View lookup cache
    private static class ViewHolder {
        TextView rackName;
        TextView rackOccupancy;
    }

    public RackAdapter(Context context, ArrayList<Rack> racks) {
        super(context, R.layout.item_rack, racks);
        this.racks = racks;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        // Get the data item for this position
        Rack rack = getItem(position);
        // Check if an existing view is being reused, otherwise inflate the view
        ViewHolder viewHolder; // view lookup cache stored in tag
        if (convertView == null) {
            // If there's no view to re-use, inflate a brand new view for row
            viewHolder = new ViewHolder();
            LayoutInflater inflater = LayoutInflater.from(getContext());
            convertView = inflater.inflate(R.layout.item_rack, parent, false);
            viewHolder.rackName = (TextView) convertView.findViewById(R.id.rackName);
            viewHolder.rackOccupancy = (TextView) convertView.findViewById(R.id.rackOccupancy);
            // Cache the viewHolder object inside the fresh view
            convertView.setTag(viewHolder);
        } else {
            // View is being recycled, retrieve the viewHolder object from tag
            viewHolder = (ViewHolder) convertView.getTag();
        }
        // Populate the data from the data object via the viewHolder object
        // into the template view.
        viewHolder.rackName.setText(rack.name);
        viewHolder.rackOccupancy.setText(rack.occupancy);
        // Return the completed view to render on screen
        return convertView;
    }

    public Rack getItem(int position){
        return racks.get(position);
    }
}