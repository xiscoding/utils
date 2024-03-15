%  This function reads a ROSbag and looks for pointcloud, image, and
%  GNSS data.  If any data is found, it is reformatted and written
%  back out as a .mat file.

%  6/28/2022

%  --load a rosbag containing lidar data

[file,mypath] = uigetfile('*.bag','multiselect','on');

if iscell(file)
    NUMFILES = length(file);
else
    if (file == 0), return, end
    
    NUMFILES = 1;
    file = {file};
end


%  --process each requested rosbag

clc

for fileno = 1:NUMFILES
    %  --load the next rosbag
    
    clear bag message_types message_names pc img pc_bag img_bag img_msgs pc_msgs
    clear gps pos_bag pos_msgs head head_bag head_msgs save_fields field_names
    
    nextfile = file{fileno};
    
    fprintf('\n----- %g of %g -----\n',fileno,NUMFILES);
    fprintf('Loading %s...please be patient\n',nextfile);
    
    k = strfind(nextfile,'.bag');
    fileroot = nextfile(1:k-1);
    
    bag = rosbag(strcat(mypath,nextfile));
    
    
    %  --get the names and types of each topic in the ROSbag
    
    message_types = string(bag.AvailableTopics.MessageType);
    message_names = string(bag.AvailableTopics.Properties.RowNames);
    
    
    %  --look for pointcloud messages
    
    k = contains(message_types, 'PointCloud2');  % Boolean array for all messages
    
    if ~any(k)
        %  --no pointcloud messages were found
        
        fprintf('No PointCloud2 message types were found\n');
        pc = [];
        
    else
        %  --check for more than one pointcloud data type
        k = find(k);  % indices to all pointcloud2 message types
        
        if (length(k) > 1), k = k(1); end   % limit to only one message
        
        fprintf('Reading %s\n',message_names(k));
        
        pc_bag = select(bag, "Topic", message_names(k));
        
        
        %  --extract ROS messages from the bag
        
        pc_msgs = readMessages(pc_bag);
        
        
        %  --read some info about the bag
        
        %     topic_name = string( pc_bag.AvailableTopics.Properties.RowNames );
        field_names = string(readAllFieldNames(pc_msgs{1}));
        
        
        %  --initialize and repackage the data structure
        
        N_msgs = length(pc_msgs);
        N_fields = length(field_names);
        pc = struct([]);
        t = zeros(N_msgs, 1);
        
        for i = 1:N_msgs
            for j = 1:N_fields
                pc(i).(field_names(j)) = readField(pc_msgs{i}, field_names(j));
            end
            secs = pc_msgs{i}.Header.Stamp.Sec;
            nsecs = pc_msgs{i}.Header.Stamp.Nsec;
            t(i) = (secs) + (nsecs * 1E-9);  %   seconds
            pc(i).t = t(i) - t(1);  % time vector starting from 0 (seconds)
            pc(i).t0 = t(1);        % ROS offset time (start of data)
        end
    end
    
    
    %  --look for image messages
    
    k = contains(message_types, 'Image');  % Boolean array for all messages
    
    if ~any(k)
        %  --no image messages were found
        
        fprintf('No Image message types were found\n');
        img = [];
        
    else
        %  --check for more than one image data type
        k = find(k);  % indices to all image message types
        
        if (length(k) > 1), k = k(1); end   % limit to only one message
        
        fprintf('Reading %s\n',message_names(k));
        
        img_bag = select(bag, "Topic", message_names(k));
        
        
        %  --extract ROS messages from the bag
        
        img_msgs = readMessages(img_bag);
        
        
        %  --read some info about the bag
        
        %     topic_name = string( img_bag.AvailableTopics.Properties.RowNames );
        
        
        %  --initialize and repackage the data structure
        
        N_msgs = length(img_msgs);
        img = struct([]);
        t = zeros(N_msgs, 1);
        
        for i = 1:N_msgs
            img(i).frame = readImage(img_msgs{i});
            
            secs = img_msgs{i}.Header.Stamp.Sec;
            nsecs = img_msgs{i}.Header.Stamp.Nsec;
            t(i) = (secs) + (nsecs * 1E-9);  %   seconds
            img(i).t = t(i) - t(1);  % time vector starting from 0 (seconds)
            img(i).t0 = t(1);        % ROS offset time (start of data)
        end
    end

    
    %  --look for NovatelPosition messages
    
    k = contains(message_types, 'NovatelPosition');  % Boolean array for all messages
    
    if ~any(k)
        %  --no NovatelPosition messages were found
        
        fprintf('No NovatelPosition message types were found\n');
        pos = [];
        
    else
        %  --check for more than one NovatelPosition data type
        k = find(k);  % indices to all NovatelPosition message types
        
        if (length(k) > 1), k = k(1); end   % limit to only one message
        
        fprintf('Reading %s\n',message_names(k));
        
        pos_bag = select(bag, "Topic", message_names(k));
        
        
        %  --extract ROS messages from the bag
        
        pos_msgs = readMessages(pos_bag,'DataFormat','struct');
        
        
        %  --read some info about the bag
        
        field_names = fields(pos_msgs{1});
        
        
        %  --initialize and repackage the data structure
        
        save_fields = {'Lat','Lon','Height','Undulation','LatSigma', ...
            'LonSigma','HeightSigma'};
        
        N_msgs = length(pos_msgs);
        N_fields = length(save_fields);
        pos = struct([]);
        t = zeros(N_msgs, 1);
        
        for i = 1:N_msgs
            for j = 1:N_fields
                eval(char(strcat('pos(',int2str(i),').',save_fields(j), ...
                    ' = pos_msgs{',int2str(i),'}.',save_fields(j),';')));
            end
            secs = pos_msgs{i}.NovatelMsgHeader.GpsSeconds;
            t(i) = secs;  %   seconds
            pos(i).t = t(i) - t(1);  % time vector starting from 0 (seconds)
            pos(i).t0 = t(1);        % ROS offset time (start of data)
        end
    end
    
    
    %  --look for NovatelHeading2 messages
    
    k = contains(message_types, 'NovatelHeading2');  % Boolean array for all messages
    
    if ~any(k)
        %  --no NovatelHeading2 messages were found
        
        fprintf('No NovatelHeading2 message types were found\n');
        head = [];
        
    else
        %  --check for more than one NovatelHeading2 data type
        k = find(k);  % indices to all NovatelHeading2 message types
        
        if (length(k) > 1), k = k(1); end   % limit to only one message
        
        fprintf('Reading %s\n',message_names(k));
        
        head_bag = select(bag, "Topic", message_names(k));
        
        
        %  --extract ROS messages from the bag
        
        head_msgs = readMessages(head_bag,'DataFormat','struct');
        
        
        %  --read some info about the bag
        
        field_names = fields(head_msgs{1});
        
        
        %  --initialize and repackage the data structure
        
        save_fields = {'BaselineLength','Heading','Pitch','HeadingSigma', ...
            'PitchSigma'};
        
        N_msgs = length(head_msgs);
        N_fields = length(save_fields);
        head = struct([]);
        t = zeros(N_msgs, 1);
        
        for i = 1:N_msgs
            for j = 1:N_fields
                eval(char(strcat('head(',int2str(i),').',save_fields(j), ...
                    ' = head_msgs{',int2str(i),'}.',save_fields(j),';')));
            end
            secs = head_msgs{i}.NovatelMsgHeader.GpsSeconds;
            t(i) = secs;  %   seconds
            head(i).t = t(i) - t(1);  % time vector starting from 0 (seconds)
            head(i).t0 = t(1);        % ROS offset time (start of data)
        end
    end

    
    %  --save the extracted data
    
    savefile = strcat(mypath,fileroot,'.mat');
    fprintf('Saving %s\n\n',strcat(fileroot,'.mat'));
    
    if (exist(savefile,'file') == 0)
        %  --create the file if it doesn't exist
        save(savefile,'fileroot');
    end
    
    if ~isempty(pc), save(savefile,'pc','-append'), end
    if ~isempty(img), save(savefile,'img','-append'), end
    if ~isempty(pos), save(savefile,'pos','-append'), end
    if ~isempty(head), save(savefile,'head','-append'), end
    
end

return