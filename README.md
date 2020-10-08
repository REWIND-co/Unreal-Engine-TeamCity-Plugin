# Unreal Engine TeamCity Plugin

To use the REWIND TeamCity Unreal Engine Plugin, you must first make sure that you have the Plugin installed as a project level plugin by dropping it in the Plugins directory adjacent to your .uproject.

To then enable the plugin, simply go to Edit -> Plugins inside of Unreal and go to the Rewind tab and check 'enabled'. You will need to restart Unreal before you can use the plugin.

![](https://lh5.googleusercontent.com/Curf7hSDj99PWUXzu3qq2eKW8zDck86wnAcM87t-tf9_edQwxoU0P3NK_l7tS6TqSkZN1KmQrAtH-hzoHGedH1eEoEjVJVAkcRRD6nA8nVn0K0R41hw=w1280)

Once you have validated that the TeamCity plugin is enabled, you can now access the TeamCity submenu under 'Window' in the editor. There are two menu items - Personal Build and Build History.

### Personal Build tab

The Personal Build tab is where you can select the changes you wish to commit to a personal build, and run the personal build. You will first need to ensure you are connected to source control in Unreal, and then click on the red button at the top right of the tab. This will open the TeamCity settings window. Here, you should type in the server address (Typically [http://build.rewind.local:8090/](http://www.google.com/url?q=http%3A%2F%2Fbuild.rewind.local%3A8090%2F&sa=D&sntz=1&usg=AFQjCNFDadXf1bhOdPRPAX6GegzeoVYBKA) , this will automatically be filled in for you), and then your TeamCity login credentials.

![](https://lh4.googleusercontent.com/JJGjoq72yZBb3cUOHcYWtNlOfHs_LlE4WzU-TRVwdA71fI20HpQ043pJfib3-Vrd7LVxR0uFIXcnx4_yK_kH0zm-Ss7RcU8XEqWZz4DmeX-9nIvHI4AI=w1280)

After clicking on Connect, you should be greeted with a green icon, and a list of changes (if any) that you have made locally that are tracked by source control. Here, you can select any of the changes you wish to commit to a personal build, then click on the 'run' button at the top left to start a new personal build. If you make any new changes and they are not being reflected in this window, make sure that they are saved and tracked by source control, and click the 'refresh' button to refresh the view.

![](https://lh6.googleusercontent.com/DSZ2WU33DuBAJvpXAgYBqTovzEJlFOIpxKsY-0EpAhtqFc7KU373M3TU2aJYU85pbkG51qF4L5IumOKTI41arTYyEGxUCe7puEj74iDuJqVnMsJvLK3h=w1280)

When you click the 'run' button, you should be greeted with a view of all of the possible build configurations for your changes. You can select as many as you like, to test your changes on multiple configurations. You should then type a description explaining your changes (this does not need to follow the same pattern as usual commits as your changes will not be integrated to the main branch, however it should be descriptive enough for you to be able to understand the commit at a glance), and then hit 'Run Personal Build'.

![](https://lh6.googleusercontent.com/nGE47XXIS2t86qjUOF6XHvNZJfSDsoRmpsEytnP063RhuLXrskDpJ_-gxfzGYjBIayF7gs0gD9XFOdcMpQ2Z-F115M_r7T5N2CYbQz99SINEcGGUE9U=w1280)

When you click Run Personal Build, the window will close and a build notification will appear for each of the build configurations you've selected. You can open the Team City build log for each of these by clicking on the 'Show TeamCity Build log' link on the notification. This will open a browser window pointing at the build page for your personal build. Here you can track the status of your build on the build agent as you would if this was a normal Team City triggered build. Alternatively, you can leave the build(s) running and continue working. You will be notified again on success/failure of the build.

You can also cancel the build by clicking the 'cancel' button.

![](https://lh3.googleusercontent.com/rbrCPBaAghNEeW6XwucVm6x4JHScvzPEfst4MozhDQUFbB5CCle6OdBAiN8M3eNOCN-ATZ5QoFmAwYNm6PcQuQTpJSpEYBwMSem2g1eCdkQNW__zBs4D=w1280)

### Build History tab

The Build History tab surfaces all of the personal builds that you have triggered for this project. The view is filtered to only show your own personal builds, and displays the status of the build on the left (success/cancelled/failed). You can click on either the time started or the build type links to open the build page for any of these builds.

![](https://lh5.googleusercontent.com/DkSBr_OHfQfpktSDrn7TVTlpcp92DUXYLlv0B2tKfA1_gBXxm8vdW-zGhlv28kktEd27o7kfObtHuBpDJwcP9UdibdkWD3PexH05tTqXmYWuc7frW_9I=w1280)
