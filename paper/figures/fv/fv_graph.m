%
% bar plot with per-class AP results for HWD2
%


if 1 % FLANN
  
  % Quantization for actioncliptest00775 1347 frames, 53.8 sec. (380014 features)
  %
  % kd(4) + checks(32) 25.71 secs ->  52.3921 fps (mAP 0.564032)
  % kd(4) + checks(16) 15.80 sec. ->  85.2532     (mAP 0.557820)
  % kd(4) + checks(8)  10.82 secs -> 124.4917     (mAP 0.552988)
  % kd(4) + checks(4)   7.57 secs -> 177.9392     (mAP 0.542571)
  % kd(2) + checks(16) 16.56 secs ->  81.3406     (mAP 0.556588)
  % kd(2) + checks(8)  11.13 secs -> 121.0243     (mAP 0.549832)
  % kd(2) + checks(4)   6.36 secs -> 211.7925     (mAP 0.538391)
  %
  %%
  %
  % NN time estimation:
  %a=single(rand(4000,100));
  %b=single(rand(38000,100));
  %tic, d=eucliddist(b,a); [mv,mi]=min(d,[],2); toc
  %Elapsed time is 3.114615 seconds.
  %
  % 3.114615 * 4 * 10 = 124.5846 secs -> 10.8119 fps (mAP 0.562)
  
  nn=[10.8119 0.562];
  fv = [1347 / 243.59, 0.575457];
  
  %kd8checks={'ch(32)','ch(16)','ch(8)','ch(4)'};  
  kd8=[34.6272 0.563959
       57.1732 0.560811
       102.2003 0.553482
       116.9271 0.526568];
  
  %kd4checks={'ch(32)','ch(16)','ch(8)','ch(4)'};  
  kd4=[52.3921 0.564032
       85.2532 0.557820
       124.4917 0.552988
       177.9392 0.542571];
  
  %kd2checks={'ch(32)','ch(16)','ch(8)','ch(4)'};
  kd2=[60.7578  0.552528
       81.3406  0.556588
       121.0243 0.549832
       211.7925 0.538391];
   
  %fv5checks = {'ch(32)','ch(16)','ch(8)','ch(4)'};
  fv5 = [40.892532 0.582075
      56.407035 0.571156
      69.325785 0.566866
      72.419355 0.558892];
  %fv1checks = {'ch(32)','ch(16)','ch(8)','ch(4)'};
  fv1 = [ 60.053500 0.567833
      94.129979 0.563006
      131.286550 0.561079
      167.537313 0.553502];
   
   %5_4_32 22.870000 10.070000 32.940000 40.892532		mAP 0.582075 (c =1e-3)
%5_4_16 13.890000 9.990000 23.880000 56.407035		mAP 0.571156 (c=1)
%5_4_8 9.000000 10.430000 19.430000 69.325785		mAP 0.566866 (c=1)
%5_4_4 6.720000 11.880000 18.600000 72.419355		mAP 0.558892 (c=1)

%1_4_32 19.780000 2.650000 22.430000 60.053500		mAP 0.567833 (c=1)
%1_4_16 11.580000 2.730000 14.310000 94.129979		mAP 0.563006 (c=0.1-1)
%1_4_8 7.540000 2.720000 10.260000 131.286550		mAP 0.561079 (c=1)
%1_4_4 5.300000 2.740000 8.040000 167.537313			mAP 0.553502 (c=1)
  
  clf
  plot(nn(1),nn(2),'kx','LineWidth',5,'MarkerSize',25); hold on,
  plot(fv(1),fv(2),'gx','LineWidth',5,'MarkerSize',25); hold on,
  %plot(kd8(:,1),kd8(:,2),'mx-','LineWidth',7,'MarkerSize',25); hold on,
  
  plot(kd4(:,1),kd4(:,2),'rx-','LineWidth',5,'MarkerSize',25); hold on,
  plot(kd2(:,1),kd2(:,2),'bx-','LineWidth',5,'MarkerSize',25); hold on,
  plot(fv5(:,1),fv5(:,2),'mx-','LineWidth',5,'MarkerSize',25); hold on,
  plot(fv1(:,1),fv1(:,2),'cx-','LineWidth',5,'MarkerSize',25); hold on,
  
  %leg={'NN','FLANN kd(8)','FLANN kd(4)','FLANN kd(2)', 'FV knn(5)', 'FV knn(1)'};
  leg={'NN', 'FV noopt', 'FLANN kd(4)','FLANN kd(2)', 'FV kd(4)', 'VLAD kd(1)'};
  grid on, set(gca,'FontSize',18), xlabel('FPS'), ylabel('mAP');
  legend(leg)
  
  %for i=1:length(kd4checks)
  %  text(kd4(i,1),kd4(i,2)+.003,kd4checks{i},'FontSize',18);
  %end
  
  %for i=1:length(kd2checks)
  %  text(kd2(i,1)-20,kd2(i,2)-.003,kd2checks{i},'FontSize',18);
  %end
  
  %for i=1:length(fv5checks)
  %  text(kd2(i,1)-20,fv5(i,2)-.003,kd2checks{i},'FontSize',18);
  %end
  
  figure(gcf)
  
  set(gca,'Position',[0.2 .25 .7 .65])
  print('-dpdf',['../hollywood2-flann-fps-map.pdf'])
  
end

if 0 % per-class accuracy on Hollywood2
   
  % CD features HOF+MBHx+MBHy+HOG
  cdall=[0.25999 0.893469 0.603643 0.728326 0.490113 0.389536 0.327611 0.659212 0.730111 0.669413 0.293175 0.698907];
  
  % TD features HOF+MBHx+MBHy+HOG
  dtall=[0.360152 0.890615 0.568082 0.769611 0.58314 0.345157 0.565624 0.667069 0.810321 0.693584 0.241237 0.702005];
  
  clf, 
  bar([cdall mean(cdall); dtall mean(dtall)]')
  colormap(autumn)
  set(gca,'FontSize',14)
  set(gca,'XTickLabel',{})
  set(gca,'Position',[0.13 .25 .775 .45])
  ylabel('Average Precision')
  grid on
  
  cl={'AnswerPhone','DriveCar','Eat','FightPerson','GetOutCar','HandShake','HugPerson','Kiss','Run','SitDown','SitUp','StandUp','All'};
  
  for i=1:length(cl)
    ht=text(i,-.04,cl{i}); 
    set(ht,'FontSize',14,'Rotation',45,'HorizontalAlignment','right');
  end
  
  leg={'CD - HOF+MBHx+MBHy+HOG', 'DT - HOF+MBHx+MBHy+HOG'};
  legend(leg)
  
  %print('-dpdf',[respath '/' 'hollywood2-per-class-barplot.pdf'])
end
